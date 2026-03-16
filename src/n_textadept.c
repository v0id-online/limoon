/*
 * n_textadept.c - Notcurses frontend for Textadept.
 *
 * Implements the platform-dependent functions required by Textadept
 * using scinterm-notcurses as the Scintilla/Notcurses backend.
 *
 * Copyright (c) 2026
 */

#include <scinterm_notcurses.h>
#include <notcurses/notcurses.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>
#include <lauxlib.h>
#include "textadept.h"
#include "textadept_platform.h"

/* Textadept globals */
extern SciObject *focused_view, *command_entry;
extern FindButton *find_next, *find_prev, *replace, *replace_all;
extern FindOption *match_case, *whole_word, *regex, *in_files;
extern lua_State *lua;
extern int exit_status;

/* ------------------------------------------------------------------ */
/* Process implementation                                               */

typedef struct NProcess {
    pid_t pid;
    int exit_status;
    bool running;
    bool monitor_stdout, monitor_stderr;
    int stdin_fd;
    int stdout_fd;
    int stderr_fd;
    struct NProcess *next;
} NProcess;

static NProcess *active_processes = NULL;

/* ------------------------------------------------------------------ */
/* Notcurses view management                                            */

typedef struct {
    struct ncplane *plane;
    SciObject *sci;
} View;

/* Global Notcurses context */
static struct notcurses *nc = NULL;
static View *current_view = NULL;

/* ------------------------------------------------------------------ */
/* Tab bar state                                                         */

#define MAX_TABS 64
typedef struct {
    char label[256];  /* display label (filename) */
    bool used;
} TabEntry;

static TabEntry tabs[MAX_TABS];
static int num_tabs = 0;
static int active_tab = 0;
static bool tabs_shown = true;

/* Per-tab pixel positions for mouse hit-testing (set by draw_tabbar) */
static int tab_x0[MAX_TABS];    /* x of tab start */
static int tab_x1[MAX_TABS];    /* x of tab end (exclusive) */
static int tab_close_x[MAX_TABS]; /* x of the 'x' character inside [x] */

static struct ncplane *tabbar_plane = NULL;
static struct ncplane *statusbar_plane = NULL;

/* Forward declarations */
static void draw_tabbar(void);
static void draw_statusbar(void);
static void tabbar_click(int cx, bool close_btn);
static bool str_contains_ci(const char *hay, const char *needle);

/* Find & replace state */
static char find_text[256] = "";
static char repl_text[256] = "";
static bool find_options[4];       /* match_case, whole_word, regex, in_files */
static char *btn_labels[4];        /* find_next, find_prev, replace, replace_all */
static char *opt_labels[4];        /* option display labels */
static char *find_label_str = NULL;
static char *repl_label_str = NULL;
static bool find_visible = false;

/* Command entry state */
static bool command_entry_active = false;
static char command_entry_label[256] = "";
static int command_entry_height_stored = 1;
static SciObject *saved_focused_view = NULL;

/* Statusbar state */
static bool statusbar_visible = false;
static char statusbar_text0[256] = "";
static char statusbar_text1[256] = "";

/* Mouse state (Bug B) */
static int mouse_pressed_button = 0;

/* Emergency exit */
static bool want_quit = false;
static int ctrl_c_count = 0;

/* Word-level undo grouping: track active begin_undo_action per target */
static bool in_word_undo = false;
static SciObject *word_undo_target = NULL;

static void end_word_group(void) {
    if (in_word_undo && word_undo_target) {
        SS(word_undo_target, SCI_ENDUNDOACTION, 0, 0);
        in_word_undo = false;
        word_undo_target = NULL;
    }
}

/* ------------------------------------------------------------------ */
/* Timeout handling                                                     */

typedef struct Timeout {
    double interval;
    bool (*f)(int *);
    int *reference;
    double trigger_time;
    struct Timeout *next;
} Timeout;

static Timeout *timeout_list = NULL;

static void process_timeouts(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    double now = ts.tv_sec + ts.tv_nsec / 1e9;
    Timeout **pt = &timeout_list;
    while (*pt) {
        Timeout *t = *pt;
        if (now >= t->trigger_time) {
            bool repeat = t->f(t->reference);
            if (repeat) {
                t->trigger_time = now + t->interval;
                pt = &(*pt)->next;
            } else {
                *pt = t->next;
                free(t);
            }
        } else {
            pt = &(*pt)->next;
        }
    }
}

/* ------------------------------------------------------------------ */
/* Process monitoring (Bug G)                                           */

static void drain_proc_fd(NProcess *np, int fd, bool is_stdout) {
    if (fd < 0) return;
    char buf[4096];
    ssize_t n;
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        bool monitoring = is_stdout ? np->monitor_stdout : np->monitor_stderr;
        if (monitoring)
            process_output((Process *)np, buf, (size_t)n, is_stdout);
    }
}

static void monitor_processes(void) {
    if (!lua) return;
    NProcess **pp = &active_processes;
    while (*pp) {
        NProcess *np = *pp;
        if (!np->running) { pp = &np->next; continue; }

        /* Non-blocking reads from stdout/stderr */
        drain_proc_fd(np, np->stdout_fd, true);
        drain_proc_fd(np, np->stderr_fd, false);

        /* Check if process has exited */
        int status;
        pid_t result = waitpid(np->pid, &status, WNOHANG);
        if (result > 0) {
            /* Drain remaining output */
            drain_proc_fd(np, np->stdout_fd, true);
            drain_proc_fd(np, np->stderr_fd, false);
            np->running = false;
            np->exit_status = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
            *pp = np->next; /* remove from list */
            process_exited((Process *)np, np->exit_status);
            /* Do not advance pp — *pp already points to next */
            continue;
        }
        pp = &np->next;
    }
}

/* ------------------------------------------------------------------ */
/* Modifier conversion                                                   */

static int nc_to_sci_mods(unsigned nc_mods) {
    int sci_mods = 0;
    if (nc_mods & NCKEY_MOD_SHIFT) sci_mods |= SCMOD_SHIFT;
    if (nc_mods & NCKEY_MOD_CTRL)  sci_mods |= SCMOD_CTRL;
    if (nc_mods & NCKEY_MOD_ALT)   sci_mods |= SCMOD_ALT;
    return sci_mods;
}

/* ------------------------------------------------------------------ */
/* Key handling                                                          */

static void handle_keypress(struct ncinput *ni) {
    if (!focused_view) return;
    uint32_t key = ni->id;
    unsigned nc_mods = ni->modifiers;
    int sci_mods = nc_to_sci_mods(nc_mods);
    int emit_key = 0;
    /* Set when a non-Kitty control code (1–26) was remapped to letter+CTRL.
     * In that case scintilla_send_key must receive the letter + NCKEY_MOD_CTRL
     * instead of the raw code, or scinterm will misread 8→BS, 9→Tab, 13→CR. */
    bool remapped_ctrl = false;

    if (key >= 1 && key <= 26) {
        /* Control codes 1–26: may arrive with or without NCKEY_MOD_CTRL.
         * Without CTRL flag (non-Kitty): 0x08/0x09/0x0a/0x0d are ambiguous
         * (Backspace/Tab/Enter sent as raw bytes, NOT Ctrl+H/I/J/M).
         * With CTRL flag (Kitty or some terminals): always Ctrl+letter. */
        if (!(nc_mods & NCKEY_MOD_CTRL)) {
            switch (key) {
                case 0x08: emit_key = SCK_BACK; break;
                case 0x09: emit_key = '\t';     break;
                case 0x0a: /* fallthrough */
                case 0x0d: emit_key = '\r';     break;
                default:
                    emit_key = (int)(key + 'a' - 1);
                    sci_mods |= SCMOD_CTRL;
                    remapped_ctrl = true;
                    break;
            }
        } else {
            /* Explicit CTRL modifier: always a Ctrl+letter, never ambiguous. */
            emit_key = (int)(key + 'a' - 1);
            /* sci_mods already has SCMOD_CTRL from nc_to_sci_mods */
            remapped_ctrl = true; /* pass letter+CTRL to scinterm, not raw code */
        }
    } else {
        switch (key) {
            case NCKEY_UP:        emit_key = SCK_UP;      break;
            case NCKEY_DOWN:      emit_key = SCK_DOWN;    break;
            case NCKEY_LEFT:      emit_key = SCK_LEFT;    break;
            case NCKEY_RIGHT:     emit_key = SCK_RIGHT;   break;
            case NCKEY_HOME:      emit_key = SCK_HOME;    break;
            case NCKEY_END:       emit_key = SCK_END;     break;
            case NCKEY_PGUP:      emit_key = SCK_PRIOR;   break;
            case NCKEY_PGDOWN:    emit_key = SCK_NEXT;    break;
            case NCKEY_DEL:       emit_key = SCK_DELETE;  break;
            case NCKEY_INS:       emit_key = SCK_INSERT;  break;
            case NCKEY_BACKSPACE: case 0x08: case 0x7f:
                                  emit_key = SCK_BACK;    break;
            case NCKEY_ESC:       emit_key = SCK_ESCAPE;  break;
            case NCKEY_ENTER: case '\r': case '\n':
                                  emit_key = '\r';        break;
            case '\t':            emit_key = '\t';        break;
            /* Ctrl+especiais fora do range 1-26 */
            case 0x00: emit_key = '@';  sci_mods |= SCMOD_CTRL; break;
            case 0x1c: emit_key = '\\'; sci_mods |= SCMOD_CTRL; break;
            case 0x1d: emit_key = ']';  sci_mods |= SCMOD_CTRL; break;
            case 0x1e: emit_key = '^';  sci_mods |= SCMOD_CTRL; break;
            case 0x1f: emit_key = '_';  sci_mods |= SCMOD_CTRL; break;
            /* Function keys: map to GDK keysyms already in Textadept's KEYSYMS table */
            case NCKEY_F01: emit_key = 0xFFBE; break;
            case NCKEY_F02: emit_key = 0xFFBF; break;
            case NCKEY_F03: emit_key = 0xFFC0; break;
            case NCKEY_F04: emit_key = 0xFFC1; break;
            case NCKEY_F05: emit_key = 0xFFC2; break;
            case NCKEY_F06: emit_key = 0xFFC3; break;
            case NCKEY_F07: emit_key = 0xFFC4; break;
            case NCKEY_F08: emit_key = 0xFFC5; break;
            case NCKEY_F09: emit_key = 0xFFC6; break;
            case NCKEY_F10: emit_key = 0xFFC7; break;
            case NCKEY_F11: emit_key = 0xFFC8; break;
            case NCKEY_F12: emit_key = 0xFFC9; break;
            default:
                if (key >= 32 && key <= 0x10FFFF)
                    emit_key = (int)key;
                else
                    return;
        }
    }

    /* Emergency exit — 3 consecutive Ctrl+C presses */
    if (emit_key == 'c' && (sci_mods & SCMOD_CTRL)) {
        if (++ctrl_c_count >= 3) { want_quit = true; return; }
    } else {
        ctrl_c_count = 0;
    }

    /* Kitty protocol envia Ctrl+letra como uppercase. Normalizar para lowercase. */
    if ((sci_mods & SCMOD_CTRL) && emit_key >= 'A' && emit_key <= 'Z')
        emit_key += 'a' - 'A';

    /* Oferecer ao Lua primeiro */
    if (emit("key", LUA_TNUMBER, emit_key, LUA_TNUMBER, sci_mods, -1)) return;

    SciObject *key_target = (command_entry_active && command_entry) ?
                            command_entry : focused_view;

    /* Fallback direto para Ctrl+Z → undo e Ctrl+Y → redo,
     * in case Lua has not handled them (binding not loaded yet, etc.). */
    if (sci_mods & SCMOD_CTRL) {
        end_word_group();
        if (emit_key == 'z') { SS(key_target, SCI_UNDO, 0, 0); return; }
        if (emit_key == 'y') { SS(key_target, SCI_REDO, 0, 0); return; }
    }

    /* Word-level undo grouping.
     * Printable non-separator chars (letters, digits, symbols except space/enter)
     * are grouped into a single undo action per word.
     * Space, tab, enter, backspace and special keys close the current group. */
    bool is_word_char = (emit_key >= 33 && emit_key <= 0x10FFFF)
                        && !(sci_mods & (SCMOD_CTRL | SCMOD_ALT));
    if (is_word_char) {
        /* Muda de target (ex: command entry <-> editor)? Fecha grupo anterior. */
        if (word_undo_target && word_undo_target != key_target)
            end_word_group();
        if (!in_word_undo) {
            SS(key_target, SCI_BEGINUNDOACTION, 0, 0);
            in_word_undo = true;
            word_undo_target = key_target;
        }
    } else {
        /* Separador, backspace, tecla especial: fecha o grupo de palavra ativo. */
        end_word_group();
    }

    /* For remapped control codes (non-Kitty), pass letter + NCKEY_MOD_CTRL
     * so scinterm does not misread 8→BS, 9→Tab, 13→CR. */
    if (remapped_ctrl)
        scintilla_send_key(key_target, emit_key, (int)(nc_mods | NCKEY_MOD_CTRL));
    else
        scintilla_send_key(key_target, (int)key, (int)nc_mods);
}

/* ------------------------------------------------------------------ */
/* Main event loop                                                       */

int main(int argc, char **argv) {
    fprintf(stderr, "[n_textadept] main starting\n");

    if (!scintilla_notcurses_init()) {
        fprintf(stderr, "[n_textadept] Failed to initialize scinterm-notcurses\n");
        return 1;
    }

    fprintf(stderr, "[n_textadept] calling init_textadept\n");
    if (!init_textadept(argc, argv)) {
        fprintf(stderr, "[n_textadept] Failed to initialize Textadept\n");
        scintilla_notcurses_shutdown();
        return 1;
    }

    if (nc) {
        notcurses_mice_enable(nc, NCMICE_ALL_EVENTS);
        /* Disable SIGTSTP/SIGINT/SIGQUIT from terminal line discipline (ISIG).
         * This prevents Ctrl+Z from suspending the process. We handle Ctrl+C
         * ourselves (3x = quit) and Ctrl+Z is undo. */
        notcurses_linesigs_disable(nc);
    }

    struct ncinput ni;
    bool running = true;
    fprintf(stderr, "[n_textadept] entering main loop\n");
    while (running && !want_quit) {
        update_ui();

        uint32_t nc_key;
        while ((nc_key = notcurses_get_nblock(nc, &ni)) != 0) {
            if (nc_key == (uint32_t)-1) break;

            if (nc_key == NCKEY_RESIZE) {
                notcurses_refresh(nc, NULL, NULL);
                if (current_view && current_view->plane && current_view->sci) {
                    unsigned rows, cols;
                    ncplane_dim_yx(notcurses_stdplane(nc), &rows, &cols);
                    /* Resize tab bar and status bar to new width */
                    if (tabbar_plane)
                        ncplane_resize_simple(tabbar_plane, 1, cols);
                    if (statusbar_plane) {
                        ncplane_resize_simple(statusbar_plane, 1, cols);
                        ncplane_move_yx(statusbar_plane, (int)rows - 1, 0);
                    }
                    unsigned view_h = rows > 2 ? rows - 2 : 1;
                    /* Shrink further if command entry is active */
                    if (command_entry_active) {
                        unsigned ce_h = (unsigned)command_entry_height_stored;
                        if (ce_h < 1) ce_h = 1;
                        view_h = view_h > ce_h ? view_h - ce_h : 1;
                    }
                    ncplane_resize_simple(current_view->plane, view_h, cols);
                    ncplane_move_yx(current_view->plane, 1, 0);
                    scintilla_resize(current_view->sci);
                    /* Resize command entry too */
                    if (command_entry_active && command_entry) {
                        unsigned ce_h = (unsigned)command_entry_height_stored;
                        if (ce_h < 1) ce_h = 1;
                        unsigned rows2, cols2;
                        ncplane_dim_yx(notcurses_stdplane(nc), &rows2, &cols2);
                        struct ncplane *ce_p = scintilla_get_plane(command_entry);
                        if (ce_p) {
                            unsigned ce_y = rows2 > 1 + ce_h ? rows2 - 1 - ce_h : 1;
                            ncplane_resize_simple(ce_p, ce_h, cols2);
                            ncplane_move_yx(ce_p, (int)ce_y, 0);
                            scintilla_resize(command_entry);
                        }
                    }
                }
                continue;
            }

            if (nckey_mouse_p(nc_key)) {
                /* Tab bar click — y==0 is the tab bar row */
                if (ni.y == 0 && tabs_shown && nc_key == NCKEY_BUTTON1) {
                    if (ni.evtype != NCTYPE_RELEASE)
                        tabbar_click(ni.x, false);
                    continue;
                }

                /* Bug B: track button state for drag detection */
                if (!current_view || !current_view->sci) continue;
                SciObject *mouse_view = (command_entry_active && command_entry) ?
                                        command_entry : current_view->sci;
                if (ni.evtype == NCTYPE_RELEASE) {
                    scintilla_send_mouse(mouse_view, SCM_RELEASE, mouse_pressed_button,
                        nc_to_sci_mods(ni.modifiers), ni.y, ni.x);
                    mouse_pressed_button = 0;
                } else if (nc_key == NCKEY_MOTION) {
                    /* Drag: only if a button is held */
                    if (mouse_pressed_button > 0)
                        scintilla_send_mouse(mouse_view, SCM_DRAG, mouse_pressed_button,
                            nc_to_sci_mods(ni.modifiers), ni.y, ni.x);
                } else {
                    int button = 1;
                    if (nc_key == NCKEY_BUTTON2)      button = 2;
                    else if (nc_key == NCKEY_BUTTON3)  button = 3;
                    else if (nc_key == NCKEY_SCROLL_UP)   button = 4;
                    else if (nc_key == NCKEY_SCROLL_DOWN) button = 5;
                    mouse_pressed_button = button;
                    scintilla_send_mouse(mouse_view, SCM_PRESS, button,
                        nc_to_sci_mods(ni.modifiers), ni.y, ni.x);
                }
            } else {
                if (ni.evtype != NCTYPE_RELEASE)
                    handle_keypress(&ni);
            }
        }

        struct timespec sleep_ts = { .tv_sec = 0, .tv_nsec = 10 * 1000000 };
        nanosleep(&sleep_ts, NULL);
    }

    fprintf(stderr, "[n_textadept] cleaning up\n");
    close_textadept();
    scintilla_notcurses_shutdown();
    fprintf(stderr, "[n_textadept] exiting with status %d\n", exit_status);
    return exit_status;
}

/* ------------------------------------------------------------------ */
/* Tab bar drawing and interaction                                       */

/* Call view:goto_buffer(_BUFFERS[tab_idx+1]) to switch to a tab */
static void switch_to_tab(int tab_idx) {
    if (!lua || tab_idx < 0 || tab_idx >= num_tabs) return;
    lua_getglobal(lua, "view");
    if (!lua_istable(lua, -1)) { lua_pop(lua, 1); return; }
    lua_getfield(lua, -1, "goto_buffer");
    if (!lua_isfunction(lua, -1)) { lua_pop(lua, 2); return; }
    lua_insert(lua, -2);               /* [fn, view] */
    lua_getglobal(lua, "_BUFFERS");
    lua_rawgeti(lua, -1, tab_idx + 1); /* [fn, view, _BUFFERS, buf] */
    lua_remove(lua, -2);               /* [fn, view, buf] */
    if (lua_pcall(lua, 2, 0, 0) != LUA_OK) {
        fprintf(stderr, "[tabs] switch error: %s\n", lua_tostring(lua, -1));
        lua_pop(lua, 1);
    }
}

/* Call _BUFFERS[tab_idx+1]:close() to close a tab */
static void close_tab(int tab_idx) {
    if (!lua || tab_idx < 0 || tab_idx >= num_tabs) return;
    lua_getglobal(lua, "_BUFFERS");
    if (!lua_istable(lua, -1)) { lua_pop(lua, 1); return; }
    lua_rawgeti(lua, -1, tab_idx + 1); /* _BUFFERS[n] */
    lua_remove(lua, -2);               /* [buf] */
    if (!lua_istable(lua, -1)) { lua_pop(lua, 1); return; }
    lua_getfield(lua, -1, "close");
    if (!lua_isfunction(lua, -1)) { lua_pop(lua, 2); return; }
    lua_insert(lua, -2);               /* [fn, buf] */
    if (lua_pcall(lua, 1, 0, 0) != LUA_OK) {
        fprintf(stderr, "[tabs] close error: %s\n", lua_tostring(lua, -1));
        lua_pop(lua, 1);
    }
}

/* Handle a click at (row=0, col=cx) on the tab bar */
static void tabbar_click(int cx, bool close_btn) {
    for (int i = 0; i < num_tabs; i++) {
        if (cx >= tab_x0[i] && cx < tab_x1[i]) {
            if (close_btn || cx == tab_close_x[i])
                close_tab(i);
            else
                switch_to_tab(i);
            return;
        }
    }
}

/* Draw all tabs onto tabbar_plane (1 row, full width). */
/* Linear interpolation for color gradient */
static void lerp_rgb(int t, int total,
                     int r1, int g1, int b1,
                     int r2, int g2, int b2,
                     int *r, int *g, int *b) {
    if (total <= 1) { *r = r1; *g = g1; *b = b1; return; }
    *r = r1 + (r2 - r1) * t / (total - 1);
    *g = g1 + (g2 - g1) * t / (total - 1);
    *b = b1 + (b2 - b1) * t / (total - 1);
}

/* Set gradient color for position t within a tab of total columns */
static void set_tab_color(struct ncplane *plane, int t, int total, bool active) {
    int r, g, b;
    if (active) {
        /* Active: deep navy → brighter blue gradient */
        lerp_rgb(t, total, 30, 65, 175, 75, 120, 230, &r, &g, &b);
        ncplane_set_bg_rgb8(plane, r, g, b);
        ncplane_set_fg_rgb8(plane, 220, 230, 255); /* near-white text */
    } else {
        /* Inactive: dark charcoal → slightly lighter gradient */
        lerp_rgb(t, total, 28, 28, 32, 52, 52, 60, &r, &g, &b);
        ncplane_set_bg_rgb8(plane, r, g, b);
        ncplane_set_fg_rgb8(plane, 145, 145, 155); /* grey text */
    }
}

static void draw_tabbar(void) {
    if (!tabbar_plane || !tabs_shown) return;

    unsigned tcols = ncplane_dim_x(tabbar_plane);
    ncplane_erase(tabbar_plane);

    /* Fill remaining area with dark background */
    ncplane_set_fg_rgb8(tabbar_plane, 80, 80, 90);
    ncplane_set_bg_rgb8(tabbar_plane, 20, 20, 24);

    int x = 0;
    for (int i = 0; i < num_tabs && x < (int)tcols; i++) {
        bool active = (i == active_tab);
        const char *lbl = tabs[i].label[0] ? tabs[i].label : "Untitled";

        /* Tab layout:  SP label SP [×] SP
         * Minimum width: 1+1+1+3+1 = 7 columns */
        int labcols = (int)strlen(lbl); /* ASCII filenames: bytes == columns */
        int tabw = 1 + labcols + 1 + 3 + 1;
        int avail = (int)tcols - x;
        if (avail < 7) break;
        if (tabw > avail) {
            labcols = avail - 6; /* re-fit label */
            tabw = avail;
        }

        tab_x0[i] = x;
        int t0 = x; /* gradient reference start */

        /* --- Draw each cell with gradient --- */

        /* Leading space */
        set_tab_color(tabbar_plane, x - t0, tabw, active);
        ncplane_putchar_yx(tabbar_plane, 0, x++, ' ');

        /* Label characters */
        for (int c = 0; c < labcols && lbl[c]; c++) {
            /* Ellipsis on last label char if label was truncated */
            bool truncated = (labcols < (int)strlen(lbl));
            set_tab_color(tabbar_plane, x - t0, tabw, active);
            if (truncated && c == labcols - 1) {
                /* Draw UTF-8 ellipsis … (U+2026, 3 bytes) */
                ncplane_putstr_yx(tabbar_plane, 0, x, "\xe2\x80\xa6");
            } else {
                ncplane_putchar_yx(tabbar_plane, 0, x, lbl[c]);
            }
            x++;
        }

        /* Space before close button */
        set_tab_color(tabbar_plane, x - t0, tabw, active);
        ncplane_putchar_yx(tabbar_plane, 0, x++, ' ');

        /* [ */
        set_tab_color(tabbar_plane, x - t0, tabw, active);
        ncplane_putchar_yx(tabbar_plane, 0, x++, '[');

        /* × — close button, slightly reddish fg */
        if (active)
            ncplane_set_fg_rgb8(tabbar_plane, 255, 120, 120);
        else
            ncplane_set_fg_rgb8(tabbar_plane, 180, 90, 90);
        /* keep gradient bg */
        { int r, g, b;
          lerp_rgb(x - t0, tabw,
                   active ? 30 : 28, active ? 65 : 28, active ? 175 : 32,
                   active ? 75 : 52, active ? 120 : 52, active ? 230 : 60,
                   &r, &g, &b);
          ncplane_set_bg_rgb8(tabbar_plane, r, g, b); }
        tab_close_x[i] = x;
        ncplane_putstr_yx(tabbar_plane, 0, x++, "\xc3\x97"); /* × U+00D7 */

        /* ] */
        set_tab_color(tabbar_plane, x - t0, tabw, active);
        ncplane_putchar_yx(tabbar_plane, 0, x++, ']');

        /* Trailing separator space — fade to dark bg */
        ncplane_set_fg_rgb8(tabbar_plane, 60, 60, 70);
        ncplane_set_bg_rgb8(tabbar_plane, 20, 20, 24);
        ncplane_putchar_yx(tabbar_plane, 0, x++, ' ');

        tab_x1[i] = x;
    }

    /* Fill rest of bar with dark background */
    ncplane_set_fg_default(tabbar_plane);
    ncplane_set_bg_rgb8(tabbar_plane, 20, 20, 24);
    for (int cx = x; cx < (int)tcols; cx++)
        ncplane_putchar_yx(tabbar_plane, 0, cx, ' ');
    ncplane_set_bg_default(tabbar_plane);
}

/* Draw the status bar onto statusbar_plane (1 row, full width).
 * Left side: statusbar_text0 (plugin space via ui.statusbar_text).
 * Right side: statusbar_text1 (file info via ui.buffer_statusbar_text). */
static void draw_statusbar(void) {
    if (!statusbar_plane) return;

    unsigned scols = ncplane_dim_x(statusbar_plane);
    ncplane_erase(statusbar_plane);

    /* Dark background, light foreground */
    ncplane_set_bg_rgb8(statusbar_plane, 30, 30, 36);
    ncplane_set_fg_rgb8(statusbar_plane, 200, 200, 210);

    /* Fill entire row with background first */
    for (int cx = 0; cx < (int)scols; cx++)
        ncplane_putchar_yx(statusbar_plane, 0, cx, ' ');

    /* Left side: plugin/statusbar_text0 */
    const char *left = statusbar_text0;
    int llen = 0;
    if (left && left[0]) {
        ncplane_putstr_yx(statusbar_plane, 0, 1, left);
        /* Approximate display width (ASCII safe; UTF-8 sequences counted as 1) */
        for (const char *p = left; *p; p++) {
            if ((*p & 0xC0) != 0x80) llen++; /* count leading bytes only */
        }
        llen += 1; /* leading space */
    }

    /* Right side: file info / statusbar_text1 */
    const char *right = statusbar_text1;
    if (right && right[0]) {
        int rlen = 0;
        for (const char *p = right; *p; p++)
            if ((*p & 0xC0) != 0x80) rlen++;
        int rx = (int)scols - rlen - 1;
        if (rx > llen + 1) {
            /* Separator between left and right sections */
            if (llen > 0) {
                ncplane_set_fg_rgb8(statusbar_plane, 80, 80, 90);
                ncplane_putchar_yx(statusbar_plane, 0, llen + 1, '|');
                ncplane_set_fg_rgb8(statusbar_plane, 200, 200, 210);
            }
            ncplane_putstr_yx(statusbar_plane, 0, rx, right);
        }
    }

    ncplane_set_fg_default(statusbar_plane);
    ncplane_set_bg_default(statusbar_plane);
}

/* ------------------------------------------------------------------ */
/* Platform functions                                                    */

const char *get_platform(void) { return "CURSES"; }
const char *get_charset(void)  { return "UTF-8"; }

void new_window(SciObject *(*get_view)(void)) {
    fprintf(stderr, "[n_textadept] new_window called\n");

    current_view = malloc(sizeof(View));
    if (!current_view) {
        fprintf(stderr, "[n_textadept] failed to allocate view\n");
        return;
    }
    memset(current_view, 0, sizeof(View));

    SciObject *sci = get_view();
    if (!sci) {
        fprintf(stderr, "[n_textadept] get_view returned NULL\n");
        free(current_view);
        current_view = NULL;
        return;
    }

    current_view->sci = sci;
    current_view->plane = scintilla_get_plane(sci);

    if (current_view->plane) {
        nc = ncplane_notcurses(current_view->plane);
        fprintf(stderr, "[n_textadept] nc context obtained: %p\n", (void *)nc);

        unsigned rows, cols;
        ncplane_dim_yx(notcurses_stdplane(nc), &rows, &cols);
        unsigned view_h = rows > 2 ? rows - 2 : 1;
        ncplane_resize_simple(current_view->plane, view_h, cols);
        ncplane_move_yx(current_view->plane, 1, 0);
        scintilla_resize(sci);

        /* Create the dedicated tab bar plane (row 0, full width, 1 row) */
        struct ncplane_options tbopt = {
            .y = 0, .x = 0, .rows = 1, .cols = cols, .name = "tabbar",
        };
        tabbar_plane = ncplane_create(notcurses_stdplane(nc), &tbopt);

        /* Create the status bar plane (last row, full width, 1 row) */
        struct ncplane_options sbopt = {
            .y = (int)rows - 1, .x = 0, .rows = 1, .cols = cols, .name = "statusbar",
        };
        statusbar_plane = ncplane_create(notcurses_stdplane(nc), &sbopt);
    }

    /* Bug J: initialize find/replace button and option pointers.
     * These must be done here (not statically) because the pointers are typed
     * as void* and arrays cannot be statically initialized through them. */
    find_next  = &btn_labels[0];
    find_prev  = &btn_labels[2];
    replace    = &btn_labels[1];
    replace_all = &btn_labels[3];
    match_case = &find_options[0];
    whole_word = &find_options[1];
    regex      = &find_options[2];
    in_files   = &find_options[3];

    fprintf(stderr, "[n_textadept] calling focus_view\n");
    focus_view(sci);
}

void set_title(const char *title) {
    if (title) fprintf(stderr, "\x1b]0;%s\x07", title);
}

bool is_maximized(void) { return false; }
void set_maximized(bool maximize) { (void)maximize; }

void get_size(int *w, int *h) {
    if (!nc) { if (w) *w = 80; if (h) *h = 24; return; }
    struct ncplane *std = notcurses_stdplane(nc);
    if (w) *w = (int)ncplane_dim_x(std);
    if (h) *h = (int)ncplane_dim_y(std);
}
void set_size(int width, int height) { (void)width; (void)height; }

SciObject *new_scintilla(void (*notified)(SciObject *, int, SCNotification *, void *)) {
    SciObject *sci = scintilla_new(notified, NULL);
    if (sci) {
        struct ncplane *p = scintilla_get_plane(sci);
        if (p) ncplane_move_bottom(p);
    }
    return sci;
}

void focus_view(SciObject *view) {
    if (!view) return;
    if (focused_view && focused_view != view) {
        scintilla_set_focus(focused_view, false);
        struct ncplane *old_p = scintilla_get_plane(focused_view);
        if (old_p) ncplane_move_bottom(old_p);
    }
    struct ncplane *new_p = scintilla_get_plane(view);
    if (new_p) ncplane_move_top(new_p);
    scintilla_set_focus(view, true);
    focused_view = view;
}

sptr_t SS(SciObject *view, int message, uptr_t wparam, sptr_t lparam) {
    return scintilla_send_message(view, message, wparam, lparam);
}

/* ------------------------------------------------------------------ */
/* Split/pane (Bug H — stub: proper pane tree requires significant work) */

void split_view(SciObject *view, SciObject *view2, bool vertical) {
    /* TODO: proper split pane management */
    (void)view; (void)view2; (void)vertical;
}

bool unsplit_view(SciObject *view, void (*delete_view)(SciObject *)) {
    (void)view; (void)delete_view;
    return false;
}

void delete_scintilla(SciObject *view) { scintilla_delete(view); }

Pane *get_top_pane(void) { return NULL; }
PaneInfo get_pane_info(Pane *pane) { (void)pane; PaneInfo i = {0}; return i; }
PaneInfo get_parent_pane_info(PaneInfo info) { (void)info; PaneInfo i = {0}; return i; }
PaneInfo get_pane_info_from_view(SciObject *view) {
    (void)view; PaneInfo i = {0}; return i;
}
void set_pane_split_pos(Pane *pane, int pos) { (void)pane; (void)pos; }

/* ------------------------------------------------------------------ */
/* Tab functions                                                         */

void show_tabs(bool show) {
    (void)show;
    tabs_shown = true; // always keep tabs visible
    if (tabbar_plane)
        draw_tabbar();
}

void add_tab(void) {
    if (num_tabs >= MAX_TABS) return;
    tabs[num_tabs].label[0] = '\0';
    tabs[num_tabs].used = true;
    num_tabs++;
}

void set_tab(int index) {
    if (index >= 0) {
        end_word_group(); /* close any open word undo group on buffer switch */
        active_tab = index;
    }
}

void set_tab_label(int index, const char *text) {
    if (index < 0 || index >= num_tabs) return;
    if (text)
        snprintf(tabs[index].label, sizeof(tabs[index].label), "%s", text);
    else
        tabs[index].label[0] = '\0';
}

void move_tab(int from, int to) {
    if (from < 0 || from >= num_tabs || to < 0 || to >= num_tabs || from == to) return;
    TabEntry tmp = tabs[from];
    if (from < to)
        memmove(&tabs[from], &tabs[from + 1], (to - from) * sizeof(TabEntry));
    else
        memmove(&tabs[to + 1], &tabs[to], (from - to) * sizeof(TabEntry));
    tabs[to] = tmp;
}

void remove_tab(int index) {
    if (index < 0 || index >= num_tabs) return;
    memmove(&tabs[index], &tabs[index + 1], (num_tabs - index - 1) * sizeof(TabEntry));
    num_tabs--;
    if (active_tab >= num_tabs && num_tabs > 0) active_tab = num_tabs - 1;
}

/* ------------------------------------------------------------------ */
/* Find & replace (Bug J)                                               */

const char *get_find_text(void)  { return find_text; }
const char *get_repl_text(void)  { return repl_text; }

void set_find_text(const char *text) {
    if (text) snprintf(find_text, sizeof(find_text), "%s", text);
    else find_text[0] = '\0';
}
void set_repl_text(const char *text) {
    if (text) snprintf(repl_text, sizeof(repl_text), "%s", text);
    else repl_text[0] = '\0';
}
void add_to_find_history(const char *text) { (void)text; }
void add_to_repl_history(const char *text) { (void)text; }
void set_entry_font(const char *name)      { (void)name; }

/* Bug J: is_checked/toggle use bool* semantics like the curses frontend */
bool is_checked(FindOption *option)  { return *(bool *)option; }
void toggle(FindOption *option, bool on) { *(bool *)option = on; }

void set_find_label(const char *text) {
    free(find_label_str);
    find_label_str = text ? strdup(text) : NULL;
}
void set_repl_label(const char *text) {
    free(repl_label_str);
    repl_label_str = text ? strdup(text) : NULL;
}
void set_button_label(FindButton *button, const char *text) {
    /* btn_labels[0..3] correspond to find_next, replace, find_prev, replace_all */
    for (int i = 0; i < 4; i++) {
        if (button == &btn_labels[i]) {
            free(btn_labels[i]);
            btn_labels[i] = text ? strdup(text) : NULL;
            return;
        }
    }
}
void set_option_label(FindOption *option, const char *text) {
    bool *opt = (bool *)option;
    int idx = (int)(opt - find_options);
    if (idx < 0 || idx >= 4) return;
    free(opt_labels[idx]);
    opt_labels[idx] = text ? strdup(text) : NULL;
}

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
/* Find bar — redesigned: fully clickable, 3 rows, Highlight All       */

/* Action codes for find bar hit-testing */
#define FB_FIND_ENTRY  0
#define FB_REPL_ENTRY  1
#define FB_PREV        2
#define FB_NEXT        3
#define FB_HIALL       4
#define FB_CLOSE       5
#define FB_REPLACE     6
#define FB_REPLALL     7
#define FB_OPT0       10
#define FB_OPT1       11
#define FB_OPT2       12
#define FB_OPT3       13

/* Tab-navigation focus order (matches FB_ values for entries/buttons) */
#define FOCUS_FIND_ENTRY  FB_FIND_ENTRY   /* 0 */
#define FOCUS_PREV        FB_PREV         /* 2 */
#define FOCUS_NEXT        FB_NEXT         /* 3 */
#define FOCUS_HIALL       FB_HIALL        /* 4 */
#define FOCUS_CLOSE       FB_CLOSE        /* 5 */
#define FOCUS_REPL_ENTRY  FB_REPL_ENTRY   /* 1 */
#define FOCUS_REPLACE     FB_REPLACE      /* 6 */
#define FOCUS_REPLALL     FB_REPLALL      /* 7 */
/* Tab cycle order (indices into this array): */
static const int focus_order[] = {
    FOCUS_FIND_ENTRY, FOCUS_PREV, FOCUS_NEXT, FOCUS_HIALL, FOCUS_CLOSE,
    FOCUS_REPL_ENTRY, FOCUS_REPLACE, FOCUS_REPLALL
};
#define FOCUS_COUNT ((int)(sizeof(focus_order)/sizeof(focus_order[0])))

typedef struct { int row, x0, x1, action; } FBHit;

/* Color helpers — colors in 0xRRGGBB for readability */
static void fb_fg(struct ncplane *p, uint32_t rgb) {
    ncplane_set_fg_rgb8(p, (rgb>>16)&0xFF, (rgb>>8)&0xFF, rgb&0xFF);
}
static void fb_bg(struct ncplane *p, uint32_t rgb) {
    ncplane_set_bg_rgb8(p, (rgb>>16)&0xFF, (rgb>>8)&0xFF, rgb&0xFF);
}

/* Draw one button " label " at (row, x), register hit, return next x.
 * When focused=true, colors are inverted to show keyboard focus. */
static int fb_draw_button(struct ncplane *p, int row, int x,
                          const char *label, int display_w,
                          uint32_t fg, uint32_t bg,
                          FBHit *hits, int *nhits, int action, bool focused)
{
    int bw = display_w + 2; /* space + label + space */
    fb_fg(p, focused ? 0x1E1E2E : fg);
    fb_bg(p, focused ? fg : bg);
    ncplane_putchar_yx(p, row, x, ' ');
    ncplane_putstr_yx(p, row, x + 1, label);
    ncplane_putchar_yx(p, row, x + 1 + display_w, ' ');
    if (*nhits < 32) hits[(*nhits)++] = (FBHit){row, x, x + bw, action};
    return x + bw + 1; /* +1 gap after button */
}

/* Draw a text entry field at (row, x) with width w.
 * Renders text with scrolling if needed and shows cursor.
 * Returns the x after the closing ']'. */
static int fb_draw_entry(struct ncplane *p, int row, int x, int w,
                         const char *text, int cur, bool focused,
                         uint32_t fg_text, uint32_t bg_entry, uint32_t c_border,
                         uint32_t c_cursor,
                         FBHit *hits, int *nhits, int action)
{
    int len = (int)strlen(text);
    int off = (cur >= w) ? cur - w + 1 : 0;

    /* Opening bracket */
    fb_fg(p, focused ? c_border : 0x45475A);
    fb_bg(p, bg_entry);
    ncplane_putchar_yx(p, row, x, focused ? '\xe2' : '['); /* draw later */
    /* Actually use plain bracket always, color indicates focus */
    ncplane_putchar_yx(p, row, x, '[');

    /* Characters */
    for (int i = 0; i < w; i++) {
        int ci = off + i;
        bool is_cur = focused && (ci == cur);
        fb_fg(p, is_cur ? 0x1E1E2E : fg_text);
        fb_bg(p, is_cur ? c_cursor : bg_entry);
        char ch = (ci < len) ? text[ci] : ' ';
        ncplane_putchar_yx(p, row, x + 1 + i, ch);
    }

    /* Closing bracket */
    fb_fg(p, focused ? c_border : 0x45475A);
    fb_bg(p, bg_entry);
    ncplane_putchar_yx(p, row, x + 1 + w, ']');

    if (*nhits < 32) hits[(*nhits)++] = (FBHit){row, x, x + w + 2, action};
    return x + w + 2;
}

/* Highlight all occurrences of the current find text via Lua. */
static void fb_highlight_all(const char *text) {
    snprintf(find_text, sizeof(find_text), "%s", text);
    /* Enable highlight_all_matches and trigger a find_text_changed event */
    luaL_dostring(lua,
        "ui.find.highlight_all_matches = true\n"
        "events.emit(events.FIND_TEXT_CHANGED)\n");
}

/* Draw the 3-row find bar. Populates hits table. */
static void draw_findbar(struct ncplane *fp, int cols,
                         const char *ftext, int fcur,
                         const char *rtext, int rcur,
                         int focus_idx,   /* FOCUS_* constant */
                         FBHit *hits, int *nhits)
{
    *nhits = 0;
    ncplane_erase(fp);

    /* ── Palette (Catppuccin Mocha-inspired) ── */
    const uint32_t C_BAR      = 0x1E1E2E; /* bar background */
    const uint32_t C_ENTRY    = 0x11111B; /* text entry bg */
    const uint32_t C_BORDER   = 0x89B4FA; /* focused entry border */
    const uint32_t C_LABEL    = 0x6C7086; /* "Find:" / "Repl:" */
    const uint32_t C_TEXT     = 0xCDD6F4; /* entry text */
    const uint32_t C_CURSOR   = 0x89B4FA; /* cursor highlight */
    const uint32_t C_BTN      = 0x313244; /* button background */
    const uint32_t C_NAV      = 0x89B4FA; /* Prev/Next — blue */
    const uint32_t C_HIALL    = 0xF9E2AF; /* Highlight All — yellow */
    const uint32_t C_CLOSE    = 0xF38BA8; /* Close — pink/red */
    const uint32_t C_REPL     = 0xA6E3A1; /* Replace buttons — green */
    const uint32_t C_OPT_ON   = 0xA6E3A1; /* checked option — green */
    const uint32_t C_OPT_OFF  = 0x585B70; /* unchecked — grey */

    /* Option names */
    const char *onames[4];
    onames[0] = opt_labels[0] ? opt_labels[0] : "Case";
    onames[1] = opt_labels[1] ? opt_labels[1] : "Word";
    onames[2] = opt_labels[2] ? opt_labels[2] : "Regex";
    onames[3] = opt_labels[3] ? opt_labels[3] : "Files";

    /* Button labels & display widths (ASCII + fixed-width Unicode symbols) */
    /* ◀ = U+25C4 (3 bytes, 1 cell); ▶ = U+25BA (3 bytes, 1 cell) */
    const char *B_PREV  = "\xe2\x97\x80 Prev";  /* ◀ Prev */ int W_PREV  = 6;
    const char *B_NEXT  = "Next \xe2\x96\xba";  /* Next ▶ */ int W_NEXT  = 6;
    const char *B_HIALL = "Hi All";              /* Hi All  */ int W_HIALL = 6;
    const char *B_CLOSE = "\xc3\x97";            /* ×       */ int W_CLOSE = 1;
    const char *B_REPL  = "Replace";             /* Replace */ int W_REPL  = 7;
    const char *B_RALL  = "Rpl All";             /* Rpl All */ int W_RALL  = 7;

    /* Row 0 button block width: each button = W+2 (spaces) + 1 (gap) */
    int btn0_w = (W_PREV+2+1) + (W_NEXT+2+1) + (W_HIALL+2+1) + (W_CLOSE+2);
    /* Label width */
    int lw = 6; /* "Find: " or "Repl: " */
    /* Entry width: fills space between label+entry brackets and button block */
    int entry_w = cols - lw - 2 - 1 - btn0_w; /* "[" + w + "]" + gap */
    if (entry_w < 8) entry_w = 8;

    /* ── Fill all 3 rows with bar background ── */
    fb_fg(fp, C_LABEL); fb_bg(fp, C_BAR);
    for (int r = 0; r < 3; r++)
        for (int c = 0; c < cols; c++)
            ncplane_putchar_yx(fp, r, c, ' ');

    /* ── Row 0: Find entry + navigation buttons ── */
    fb_fg(fp, C_LABEL); fb_bg(fp, C_BAR);
    ncplane_putstr_yx(fp, 0, 0, "Find: ");
    int x = lw;
    x = fb_draw_entry(fp, 0, x, entry_w, ftext, fcur, focus_idx == FOCUS_FIND_ENTRY,
                      C_TEXT, C_ENTRY, C_BORDER, C_CURSOR,
                      hits, nhits, FB_FIND_ENTRY);
    x++; /* gap before buttons */
    x = fb_draw_button(fp, 0, x, B_PREV,  W_PREV,  C_NAV,   C_BTN, hits, nhits, FB_PREV,  focus_idx == FOCUS_PREV);
    x = fb_draw_button(fp, 0, x, B_NEXT,  W_NEXT,  C_NAV,   C_BTN, hits, nhits, FB_NEXT,  focus_idx == FOCUS_NEXT);
    x = fb_draw_button(fp, 0, x, B_HIALL, W_HIALL, C_HIALL, C_BTN, hits, nhits, FB_HIALL, focus_idx == FOCUS_HIALL);
        fb_draw_button(fp, 0, x, B_CLOSE, W_CLOSE, C_CLOSE, C_BTN, hits, nhits, FB_CLOSE, focus_idx == FOCUS_CLOSE);

    /* ── Row 1: Replace entry + replace buttons ── */
    fb_fg(fp, C_LABEL); fb_bg(fp, C_BAR);
    ncplane_putstr_yx(fp, 1, 0, "Repl: ");
    x = lw;
    x = fb_draw_entry(fp, 1, x, entry_w, rtext, rcur, focus_idx == FOCUS_REPL_ENTRY,
                      C_TEXT, C_ENTRY, C_BORDER, C_CURSOR,
                      hits, nhits, FB_REPL_ENTRY);
    x++;
    x = fb_draw_button(fp, 1, x, B_REPL, W_REPL, C_REPL, C_BTN, hits, nhits, FB_REPLACE, focus_idx == FOCUS_REPLACE);
        fb_draw_button(fp, 1, x, B_RALL, W_RALL, C_REPL, C_BTN, hits, nhits, FB_REPLALL, focus_idx == FOCUS_REPLALL);

    /* ── Row 2: Option checkboxes ── */
    x = 1;
    for (int i = 0; i < 4; i++) {
        bool on = find_options[i];
        int olen = (int)strlen(onames[i]);
        int ox0 = x;
        /* "[✓] " (✓ = 3 UTF-8 bytes, 1 display cell) or "[ ] " */
        fb_fg(fp, on ? C_OPT_ON : C_OPT_OFF); fb_bg(fp, C_BAR);
        ncplane_putchar_yx(fp, 2, x,     '[');
        if (on)
            ncplane_putstr_yx(fp, 2, x + 1, "\xe2\x9c\x93"); /* ✓ */
        else
            ncplane_putchar_yx(fp, 2, x + 1, ' ');
        ncplane_putstr_yx(fp, 2, x + 2, "] ");
        fb_fg(fp, on ? C_TEXT : C_OPT_OFF); fb_bg(fp, C_BAR);
        ncplane_putstr_yx(fp, 2, x + 4, onames[i]);
        int slot_w = 4 + olen + 2;
        if (*nhits < 32)
            hits[(*nhits)++] = (FBHit){2, ox0, ox0 + slot_w, FB_OPT0 + i};
        x += slot_w;
    }

    /* Position terminal cursor: inside active entry, or start of find entry
     * when a button is focused (the button highlight shows focus visually). */
    {
        int entry_row = (focus_idx == FOCUS_REPL_ENTRY) ? 1 : 0;
        int cur       = (focus_idx == FOCUS_REPL_ENTRY) ? rcur : fcur;
        int off = (cur >= entry_w) ? cur - entry_w + 1 : 0;
        int cx  = lw + 1 + cur - off;
        ncplane_cursor_move_yx(fp, entry_row, cx);
    }

    fb_fg(fp, C_TEXT); fb_bg(fp, C_BAR); /* restore defaults */
}

/* Insert a Unicode codepoint (already > 0x7e) into a text buffer at *cur. */
static void fb_insert_uni(char *buf, int bufsz, int *cur, uint32_t cp) {
    /* Encode to UTF-8 */
    char enc[5] = {0};
    int n = 0;
    if (cp < 0x80)         { enc[0] = (char)cp; n = 1; }
    else if (cp < 0x800)   { enc[0] = (char)(0xC0|(cp>>6)); enc[1]=(char)(0x80|(cp&0x3F)); n=2; }
    else if (cp < 0x10000) { enc[0]=(char)(0xE0|(cp>>12)); enc[1]=(char)(0x80|((cp>>6)&0x3F));
                              enc[2]=(char)(0x80|(cp&0x3F)); n=3; }
    else                   { enc[0]=(char)(0xF0|(cp>>18)); enc[1]=(char)(0x80|((cp>>12)&0x3F));
                              enc[2]=(char)(0x80|((cp>>6)&0x3F)); enc[3]=(char)(0x80|(cp&0x3F)); n=4; }
    int len = (int)strlen(buf);
    if (len + n >= bufsz) return;
    memmove(buf + *cur + n, buf + *cur, (size_t)(len - *cur + 1));
    memcpy(buf + *cur, enc, (size_t)n);
    *cur += n;
}

void focus_find(void) {
    if (!nc) return;
    if (find_visible) return;
    find_visible = true;

    unsigned rows, cols;
    ncplane_dim_yx(notcurses_stdplane(nc), &rows, &cols);

    /* Shrink main view by 3 rows (tabbar=1, findbar=3, statusbar=1) */
    if (current_view && current_view->plane) {
        unsigned main_h = rows > 5 ? rows - 5 : 1;
        ncplane_resize_simple(current_view->plane, main_h, cols);
        scintilla_resize(current_view->sci);
    }

    emit("find_pane_show", -1);

    /* Create find bar plane: 3 rows, above statusbar */
    int fp_y = (int)rows - 4; /* tabbar(1) + view + findbar(3) + statusbar(1) */
    if (fp_y < 1) fp_y = 1;
    struct ncplane_options fpopt = {
        .y = fp_y, .x = 0, .rows = 3, .cols = cols, .name = "findbar"
    };
    struct ncplane *fp = ncplane_create(notcurses_stdplane(nc), &fpopt);
    if (!fp) { find_visible = false; return; }
    ncplane_move_top(fp);

    /* Working copies of find/replace text */
    char ftext[256], rtext[256];
    snprintf(ftext, sizeof(ftext), "%s", find_text);
    snprintf(rtext, sizeof(rtext), "%s", repl_text);
    int fcur = (int)strlen(ftext);
    int rcur = (int)strlen(rtext);
    int focus_pos = 0; /* index into focus_order[], starts at find entry */

    FBHit hits[32];
    int nhits = 0;
    struct ncinput ni;
    bool done = false;

    while (!done) {
        /* Re-render the editor view so find results are visible */
        if (current_view && current_view->sci)
            scintilla_render(current_view->sci);
        draw_findbar(fp, (int)cols, ftext, fcur, rtext, rcur,
                     focus_order[focus_pos], hits, &nhits);
        notcurses_render(nc);
        notcurses_get_blocking(nc, &ni);
        if (ni.evtype == NCTYPE_RELEASE) continue;

        uint32_t k = ni.id;

        /* ── Mouse click ── */
        if (nckey_mouse_p(k) && k == NCKEY_BUTTON1
            && ni.evtype != NCTYPE_RELEASE) {
            /* Convert absolute coords to plane-local */
            int my = ni.y - fp_y;
            int mx = ni.x;
            if (my >= 0 && my < 3) {
                int action = -1;
                for (int i = 0; i < nhits; i++) {
                    if (hits[i].row == my && mx >= hits[i].x0 && mx < hits[i].x1) {
                        action = hits[i].action; break;
                    }
                }
                /* Sync focus_pos to match the clicked element */
                for (int fi = 0; fi < FOCUS_COUNT; fi++) {
                    if (focus_order[fi] == action) { focus_pos = fi; break; }
                }
                switch (action) {
                case FB_FIND_ENTRY:
                    /* Position cursor at click */
                    { int entry_w = (int)cols - 6 - 2 - 1 - ((6+2+1)+(6+2+1)+(6+2+1)+(1+2));
                      if (entry_w < 8) entry_w = 8;
                      int off = (fcur >= entry_w) ? fcur - entry_w + 1 : 0;
                      int ci = off + (mx - 7); /* 6(label)+1('[') = 7 */
                      int len = (int)strlen(ftext);
                      if (ci < 0) ci = 0;
                      if (ci > len) ci = len;
                      fcur = ci; }
                    break;
                case FB_REPL_ENTRY:
                    { int entry_w = (int)cols - 6 - 2 - 1 - ((6+2+1)+(6+2+1)+(6+2+1)+(1+2));
                      if (entry_w < 8) entry_w = 8;
                      int off = (rcur >= entry_w) ? rcur - entry_w + 1 : 0;
                      int ci = off + (mx - 7);
                      int len = (int)strlen(rtext);
                      if (ci < 0) ci = 0;
                      if (ci > len) ci = len;
                      rcur = ci; }
                    break;
                case FB_PREV:
                    snprintf(find_text, sizeof(find_text), "%s", ftext);
                    snprintf(repl_text, sizeof(repl_text), "%s", rtext);
                    if (find_prev) find_clicked(find_prev);
                    break;
                case FB_NEXT:
                    snprintf(find_text, sizeof(find_text), "%s", ftext);
                    snprintf(repl_text, sizeof(repl_text), "%s", rtext);
                    if (find_next) find_clicked(find_next);
                    break;
                case FB_HIALL:
                    fb_highlight_all(ftext);
                    break;
                case FB_CLOSE:
                    done = true;
                    break;
                case FB_REPLACE:
                    snprintf(find_text, sizeof(find_text), "%s", ftext);
                    snprintf(repl_text, sizeof(repl_text), "%s", rtext);
                    if (replace) find_clicked(replace);
                    break;
                case FB_REPLALL:
                    snprintf(find_text, sizeof(find_text), "%s", ftext);
                    snprintf(repl_text, sizeof(repl_text), "%s", rtext);
                    if (replace_all) find_clicked(replace_all);
                    break;
                case FB_OPT0: toggle(match_case,  !find_options[0]); break;
                case FB_OPT1: toggle(whole_word,  !find_options[1]); break;
                case FB_OPT2: toggle(regex,       !find_options[2]); break;
                case FB_OPT3: toggle(in_files,    !find_options[3]); break;
                default: break;
                }
            }
            continue;
        }

        /* ── Keyboard ── */
        int cur_focus = focus_order[focus_pos]; /* FB_* value of current focus */
        bool on_entry = (cur_focus == FOCUS_FIND_ENTRY || cur_focus == FOCUS_REPL_ENTRY);

        if (k == NCKEY_ESC || k == 0x06 /* Ctrl+F */) {
            done = true;

        } else if (k == '\t') {
            /* Tab / Shift+Tab cycle through all focusable elements */
            if (ni.modifiers & NCKEY_MOD_SHIFT)
                focus_pos = (focus_pos + FOCUS_COUNT - 1) % FOCUS_COUNT;
            else
                focus_pos = (focus_pos + 1) % FOCUS_COUNT;

        } else if (k == NCKEY_ENTER || k == '\r' || k == '\n') {
            /* Activate focused element */
            snprintf(find_text, sizeof(find_text), "%s", ftext);
            snprintf(repl_text, sizeof(repl_text), "%s", rtext);
            switch (cur_focus) {
            case FOCUS_PREV:        if (find_prev)  find_clicked(find_prev);    break;
            case FOCUS_HIALL:       fb_highlight_all(ftext);                    break;
            case FOCUS_CLOSE:       done = true;                                break;
            case FOCUS_REPLACE:     if (replace)    find_clicked(replace);      break;
            case FOCUS_REPLALL:     if (replace_all)find_clicked(replace_all);  break;
            default: /* FIND_ENTRY, NEXT → find next */
                if (find_next) find_clicked(find_next);
                break;
            }

        } else if (k == NCKEY_UP) {
            /* Move focus to find entry */
            for (int fi = 0; fi < FOCUS_COUNT; fi++)
                if (focus_order[fi] == FOCUS_FIND_ENTRY) { focus_pos = fi; break; }
        } else if (k == NCKEY_DOWN) {
            /* Move focus to replace entry */
            for (int fi = 0; fi < FOCUS_COUNT; fi++)
                if (focus_order[fi] == FOCUS_REPL_ENTRY) { focus_pos = fi; break; }

        } else if (k == NCKEY_LEFT) {
            if (on_entry) {
                int *cur = (cur_focus == FOCUS_FIND_ENTRY) ? &fcur : &rcur;
                if (*cur > 0) (*cur)--;
            }
        } else if (k == NCKEY_RIGHT) {
            if (on_entry) {
                char *tx  = (cur_focus == FOCUS_FIND_ENTRY) ? ftext : rtext;
                int  *cur = (cur_focus == FOCUS_FIND_ENTRY) ? &fcur : &rcur;
                if (*cur < (int)strlen(tx)) (*cur)++;
            }
        } else if (k == NCKEY_HOME) {
            if (cur_focus == FOCUS_FIND_ENTRY) fcur = 0;
            else if (cur_focus == FOCUS_REPL_ENTRY) rcur = 0;
        } else if (k == NCKEY_END) {
            if (cur_focus == FOCUS_FIND_ENTRY) fcur = (int)strlen(ftext);
            else if (cur_focus == FOCUS_REPL_ENTRY) rcur = (int)strlen(rtext);

        } else if (k == NCKEY_BACKSPACE || k == 0x08 || k == 0x7f) {
            /* If on a button, redirect typing to find entry */
            if (!on_entry) {
                focus_pos = 0; /* FOCUS_FIND_ENTRY is first in focus_order */
                cur_focus = FOCUS_FIND_ENTRY;
            }
            char *tx  = (cur_focus == FOCUS_FIND_ENTRY) ? ftext : rtext;
            int  *cur = (cur_focus == FOCUS_FIND_ENTRY) ? &fcur : &rcur;
            if (*cur > 0) {
                int n = 1;
                while (*cur - n > 0 && (tx[*cur - n] & 0xC0) == 0x80) n++;
                memmove(tx + *cur - n, tx + *cur,
                        strlen(tx) - (size_t)(*cur) + 1);
                *cur -= n;
                snprintf(find_text, sizeof(find_text), "%s", ftext);
                emit("find_text_changed", -1);
            }
        } else if (k == NCKEY_DEL) {
            if (!on_entry) { focus_pos = 0; cur_focus = FOCUS_FIND_ENTRY; }
            char *tx  = (cur_focus == FOCUS_FIND_ENTRY) ? ftext : rtext;
            int  *cur = (cur_focus == FOCUS_FIND_ENTRY) ? &fcur : &rcur;
            int   len = (int)strlen(tx);
            if (*cur < len) {
                int n = 1;
                while (*cur + n < len && (tx[*cur + n] & 0xC0) == 0x80) n++;
                memmove(tx + *cur, tx + *cur + n, (size_t)(len - *cur - n + 1));
                snprintf(find_text, sizeof(find_text), "%s", ftext);
                emit("find_text_changed", -1);
            }
        } else if ((k >= ' ' && k <= 0x7e) || k > 0x7e) {
            /* Printable: redirect to find entry if focus is on a button */
            if (!on_entry) { focus_pos = 0; cur_focus = FOCUS_FIND_ENTRY; }
            char *tx  = (cur_focus == FOCUS_FIND_ENTRY) ? ftext : rtext;
            int  *cur = (cur_focus == FOCUS_FIND_ENTRY) ? &fcur : &rcur;
            if (k <= 0x7e) {
                int len = (int)strlen(tx);
                if (len + 1 < (int)sizeof(ftext)) {
                    memmove(tx + *cur + 1, tx + *cur, (size_t)(len - *cur + 1));
                    tx[(*cur)++] = (char)k;
                }
            } else {
                fb_insert_uni(tx, (int)sizeof(ftext), cur, k);
            }
            snprintf(find_text, sizeof(find_text), "%s", ftext);
            emit("find_text_changed", -1);
        } else {
            /* F1-F4 toggle options */
            if      (k == NCKEY_F01) toggle(match_case, !find_options[0]);
            else if (k == NCKEY_F02) toggle(whole_word, !find_options[1]);
            else if (k == NCKEY_F03) toggle(regex,      !find_options[2]);
            else if (k == NCKEY_F04) toggle(in_files,   !find_options[3]);
        }
    }

    /* Commit final text */
    snprintf(find_text, sizeof(find_text), "%s", ftext);
    snprintf(repl_text, sizeof(repl_text), "%s", rtext);

    ncplane_destroy(fp);

    /* Restore main view size */
    if (nc && current_view && current_view->plane) {
        unsigned rows2, cols2;
        ncplane_dim_yx(notcurses_stdplane(nc), &rows2, &cols2);
        unsigned main_h = rows2 > 2 ? rows2 - 2 : 1;
        ncplane_resize_simple(current_view->plane, main_h, cols2);
        ncplane_move_yx(current_view->plane, 1, 0);
        scintilla_resize(current_view->sci);
    }
    emit("find_pane_hide", -1);
    find_visible = false;
    notcurses_render(nc);
}

/* ------------------------------------------------------------------ */
/* Command entry (Bug I)                                                 */

static void resize_views_for_command_entry(bool active) {
    if (!nc || !current_view || !current_view->plane) return;
    unsigned rows, cols;
    ncplane_dim_yx(notcurses_stdplane(nc), &rows, &cols);

    unsigned ce_h = (command_entry_height_stored > 0) ?
                    (unsigned)command_entry_height_stored : 1;

    if (active) {
        /* Main view shrinks by ce_h */
        unsigned main_h = rows > 2 + ce_h ? rows - 2 - ce_h : 1;
        ncplane_resize_simple(current_view->plane, main_h, cols);
        ncplane_move_yx(current_view->plane, 1, 0);
        scintilla_resize(current_view->sci);

        /* Position command entry above status bar */
        if (command_entry) {
            struct ncplane *ce_p = scintilla_get_plane(command_entry);
            if (ce_p) {
                unsigned ce_y = rows > 1 + ce_h ? rows - 1 - ce_h : 1;
                ncplane_resize_simple(ce_p, ce_h, cols);
                ncplane_move_yx(ce_p, (int)ce_y, 0);
                scintilla_resize(command_entry);
            }
        }
    } else {
        /* Restore main view */
        unsigned main_h = rows > 2 ? rows - 2 : 1;
        ncplane_resize_simple(current_view->plane, main_h, cols);
        ncplane_move_yx(current_view->plane, 1, 0);
        scintilla_resize(current_view->sci);

        /* Push command entry behind everything */
        if (command_entry) {
            struct ncplane *ce_p = scintilla_get_plane(command_entry);
            if (ce_p) ncplane_move_bottom(ce_p);
        }
    }
}

void focus_command_entry(void) {
    command_entry_active = !command_entry_active;
    if (command_entry_active) {
        saved_focused_view = focused_view;
        resize_views_for_command_entry(true);
        if (command_entry) {
            /* Bring command entry plane to top */
            struct ncplane *ce_p = scintilla_get_plane(command_entry);
            if (ce_p) ncplane_move_top(ce_p);
            scintilla_set_focus(command_entry, true);
            if (focused_view && focused_view != command_entry)
                scintilla_set_focus(focused_view, false);
            focused_view = command_entry;
        }
    } else {
        SS(command_entry, SCI_SETFOCUS, 0, 0);
        resize_views_for_command_entry(false);
        SciObject *restore = saved_focused_view ? saved_focused_view : focused_view;
        saved_focused_view = NULL;
        focus_view(restore);
    }
}

bool is_command_entry_active(void) { return command_entry_active; }

void set_command_entry_label(const char *text) {
    if (text) snprintf(command_entry_label, sizeof(command_entry_label), "%s", text);
    else command_entry_label[0] = '\0';
}
int  get_command_entry_height(void) {
    if (!command_entry) return 0;
    struct ncplane *p = scintilla_get_plane(command_entry);
    return p ? (int)ncplane_dim_y(p) : 0;
}
void set_command_entry_height(int height) {
    command_entry_height_stored = height;
    if (command_entry_active)
        resize_views_for_command_entry(true);
}

/* ------------------------------------------------------------------ */
/* Statusbar (Bug D)                                                     */

bool is_statusbar_visible(void) { return statusbar_visible; }
void set_statusbar_visible(bool visible) {
    if (statusbar_visible == visible) return;
    statusbar_visible = visible;
    /* Resize main view: statusbar occupies the last row.
     * Since we don't yet render the statusbar ourselves, leave view_h = rows-2
     * regardless (the -2 already accounts for both tabbar and statusbar). */
}
const char *get_statusbar_text(int bar) {
    if (bar == 0) return statusbar_text0;
    if (bar == 1) return statusbar_text1;
    return "";
}
void set_statusbar_text(int bar, const char *text) {
    if (bar == 0 && text) snprintf(statusbar_text0, sizeof(statusbar_text0), "%s", text);
    else if (bar == 1 && text) snprintf(statusbar_text1, sizeof(statusbar_text1), "%s", text);
}

/* ------------------------------------------------------------------ */
/* Menus                                                                 */

void *read_menu(lua_State *L, int index)   { (void)L; (void)index; return NULL; }
void popup_menu(void *menu, void *userdata){ (void)menu; (void)userdata; }
void set_menubar(lua_State *L, int index)  { (void)L; (void)index; }

/* ------------------------------------------------------------------ */
/* Clipboard                                                             */

char *get_clipboard_text(int *len) {
    if (!focused_view) { if (len) *len = 0; return NULL; }
    return scintilla_get_clipboard(focused_view, len);
}

/* ------------------------------------------------------------------ */
/* Timeout                                                               */

void add_timeout(double interval, bool (*f)(int *), int *reference) {
    Timeout *t = malloc(sizeof(Timeout));
    if (!t) return;
    t->interval = interval;
    t->f = f;
    t->reference = reference;
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    t->trigger_time = ts.tv_sec + ts.tv_nsec / 1e9 + interval;
    t->next = timeout_list;
    timeout_list = t;
}

/* ------------------------------------------------------------------ */
/* UI update                                                             */

void update_ui(void) {
    if (!nc) return;
    process_timeouts();
    monitor_processes();

    if (current_view && current_view->sci)
        scintilla_render(current_view->sci);

    if (command_entry_active && command_entry) {
        scintilla_render(command_entry);
        scintilla_update_cursor(command_entry);
    } else if (current_view && current_view->sci) {
        scintilla_update_cursor(current_view->sci);
    }

    draw_tabbar();
    draw_statusbar();
    notcurses_render(nc);
}

bool is_hidpi(void)     { return false; }
bool is_dark_mode(void) { return false; }

/* ------------------------------------------------------------------ */
/* message_dialog — multi-button (Bug C)                                */

int message_dialog(DialogOptions opts, lua_State *L) {
    if (!nc) return 0;
    struct ncplane *std = notcurses_stdplane(nc);
    unsigned rows, cols;
    ncplane_dim_yx(std, &rows, &cols);
    int h = 9, w = (int)cols * 2 / 3;
    if (w > 72) w = 72;
    int y = ((int)rows - h) / 2, x = ((int)cols - w) / 2;

    struct ncplane_options popt = {
        .y = y, .x = x, .rows = h, .cols = w, .name = "dialog",
    };
    struct ncplane *dplane = ncplane_create(std, &popt);
    if (!dplane) return 0;

    /* Count buttons */
    int num_btn = 0;
    const char *btns[3];
    for (int i = 0; i < 3; i++) {
        btns[i] = opts.buttons[i] ? opts.buttons[i] : NULL;
        if (btns[i]) num_btn = i + 1;
    }
    if (num_btn == 0) { btns[0] = "OK"; num_btn = 1; }

    /* Compute total button width: " X… " + "[ X… ]" = label+2 each, gap=2 */
    int btn_widths[3] = {0};
    int total_btn_w = 0;
    for (int i = 0; i < num_btn; i++) {
        btn_widths[i] = (int)strlen(btns[i]) + 4; /* [_label_] */
        total_btn_w += btn_widths[i] + (i < num_btn - 1 ? 2 : 0); /* gap */
    }

    int btn_y = h - 3;
    int btn_x = (w - total_btn_w) / 2;
    if (btn_x < 1) btn_x = 1;

    /* Color helpers (reuse dialog plane directly) */
    const uint32_t C_BG      = 0x313244; /* Catppuccin surface0 */
    const uint32_t C_BORDER  = 0x89B4FA; /* blue */
    const uint32_t C_TITLE   = 0xCDD6F4; /* text */
    const uint32_t C_TEXT    = 0xBAC2DE; /* subtext */
    const uint32_t C_BTN_FG  = 0xCDD6F4;
    const uint32_t C_BTN_BG  = 0x45475A; /* surface2 */
    const uint32_t C_FOC_FG  = 0x1E1E2E;
    const uint32_t C_FOC_BG  = 0x89B4FA; /* focused = inverted blue */
    const uint32_t C_ACCEL   = 0xF9E2AF; /* yellow for underlined letter */

#define DLG_FG(r,g,b) ncplane_set_fg_rgb8(dplane,(r),(g),(b))
#define DLG_BG(r,g,b) ncplane_set_bg_rgb8(dplane,(r),(g),(b))
#define DLG_FGHEX(c)  DLG_FG(((c)>>16)&0xFF,((c)>>8)&0xFF,(c)&0xFF)
#define DLG_BGHEX(c)  DLG_BG(((c)>>16)&0xFF,((c)>>8)&0xFF,(c)&0xFF)

    int cur_btn = 0;
    struct ncinput ni;
    bool done = false;
    int result = 0;

    while (!done) {
        /* ── Background fill ── */
        DLG_FGHEX(C_BG); DLG_BGHEX(C_BG);
        ncplane_set_styles(dplane, NCSTYLE_NONE);
        for (int r = 0; r < h; r++)
            for (int c = 0; c < w; c++)
                ncplane_putchar_yx(dplane, r, c, ' ');

        /* ── Border ── */
        DLG_FGHEX(C_BORDER); DLG_BGHEX(C_BG);
        ncplane_putstr_yx(dplane, 0, 0, "╭");
        ncplane_putstr_yx(dplane, 0, w - 3, "╮"); /* 3-byte UTF-8 per char */
        ncplane_putstr_yx(dplane, h - 1, 0, "╰");
        ncplane_putstr_yx(dplane, h - 1, w - 3, "╯");
        for (int c = 1; c < w - 1; c++) {
            ncplane_putstr_yx(dplane, 0,     c, "─");
            ncplane_putstr_yx(dplane, h - 1, c, "─");
        }
        for (int r = 1; r < h - 1; r++) {
            ncplane_putstr_yx(dplane, r, 0,     "│");
            ncplane_putstr_yx(dplane, r, w - 1, "│");
        }

        /* ── Title ── */
        if (opts.title) {
            DLG_FGHEX(C_TITLE); DLG_BGHEX(C_BG);
            ncplane_set_styles(dplane, NCSTYLE_BOLD);
            int tx = (w - (int)strlen(opts.title)) / 2;
            if (tx < 1) tx = 1;
            ncplane_putstr_yx(dplane, 1, tx, opts.title);
            ncplane_set_styles(dplane, NCSTYLE_NONE);
        }

        /* ── Text (word-wrap at w-4) ── */
        if (opts.text) {
            DLG_FGHEX(C_TEXT); DLG_BGHEX(C_BG);
            /* Simple line-by-line: split on \n */
            const char *p = opts.text;
            int trow = 3;
            while (*p && trow < btn_y - 1) {
                const char *nl = strchr(p, '\n');
                int len = nl ? (int)(nl - p) : (int)strlen(p);
                if (len > w - 4) len = w - 4;
                char tmp[256];
                snprintf(tmp, sizeof(tmp), "%.*s", len, p);
                ncplane_putstr_yx(dplane, trow++, 2, tmp);
                if (!nl) break;
                p = nl + 1;
            }
        }

        /* ── Buttons ── */
        int bx = btn_x;
        for (int i = 0; i < num_btn; i++) {
            bool focused = (i == cur_btn);
            uint32_t fg = focused ? C_FOC_FG : C_BTN_FG;
            uint32_t bg = focused ? C_FOC_BG : C_BTN_BG;
            DLG_FGHEX(fg); DLG_BGHEX(bg);
            ncplane_set_styles(dplane, NCSTYLE_NONE);
            /* Opening bracket */
            ncplane_putchar_yx(dplane, btn_y, bx,     focused ? '[' : ' ');
            /* Accelerator letter (first char) — underlined, different color */
            uint32_t afg = focused ? C_FOC_FG : C_ACCEL;
            DLG_FGHEX(afg); DLG_BGHEX(bg);
            ncplane_set_styles(dplane, NCSTYLE_UNDERLINE | NCSTYLE_BOLD);
            ncplane_putchar_yx(dplane, btn_y, bx + 1, btns[i][0]);
            ncplane_set_styles(dplane, NCSTYLE_NONE);
            /* Rest of label */
            DLG_FGHEX(fg); DLG_BGHEX(bg);
            if (btns[i][1])
                ncplane_putstr_yx(dplane, btn_y, bx + 2, btns[i] + 1);
            /* Closing bracket */
            int llen = (int)strlen(btns[i]);
            ncplane_putchar_yx(dplane, btn_y, bx + 1 + llen, focused ? ']' : ' ');
            bx += btn_widths[i] + 2; /* gap */
        }

        /* ── Hint line ── */
        DLG_FGHEX(0x585B70); DLG_BGHEX(C_BG);
        ncplane_putstr_yx(dplane, h - 2, 2, "←/→ Tab · Enter · underlined letter");

        notcurses_render(nc);
        notcurses_get_blocking(nc, &ni);
        if (ni.evtype == NCTYPE_RELEASE) continue;

        uint32_t k = ni.id;
        if (k == NCKEY_LEFT)  cur_btn = (cur_btn + num_btn - 1) % num_btn;
        else if (k == NCKEY_RIGHT || k == '\t') cur_btn = (cur_btn + 1) % num_btn;
        else if (k == NCKEY_ENTER || k == '\r' || k == '\n') {
            result = cur_btn + 1; done = true;
        } else if (k == NCKEY_ESC) {
            result = 0; done = true;
        } else if (k >= ' ' && k <= 0x7e) {
            /* Accelerator: match first char of any button (case-insensitive) */
            char kc = (char)(k >= 'A' && k <= 'Z' ? k + 32 : k);
            for (int i = 0; i < num_btn; i++) {
                char bc = btns[i][0];
                if (bc >= 'A' && bc <= 'Z') bc += 32;
                if (kc == bc) { result = i + 1; done = true; break; }
            }
        }
    }

#undef DLG_FG
#undef DLG_BG
#undef DLG_FGHEX
#undef DLG_BGHEX

    ncplane_destroy(dplane);
    notcurses_render(nc);

    if (result > 0) {
        lua_pushinteger(L, result);
        return 1;
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* input_dialog                                                          */

int input_dialog(DialogOptions opts, lua_State *L) {
    if (!nc) return 0;
    struct ncplane *std = notcurses_stdplane(nc);
    unsigned rows, cols;
    ncplane_dim_yx(std, &rows, &cols);
    int h = 10, w = (int)cols * 2 / 3;
    if (w > 70) w = 70;
    int y = ((int)rows - h) / 2, x = ((int)cols - w) / 2;

    struct ncplane_options popt = {
        .y = y, .x = x, .rows = h, .cols = w, .name = "input_dialog",
    };
    struct ncplane *dplane = ncplane_create(std, &popt);
    if (!dplane) return 0;
    ncplane_set_base(dplane, " ", 0, 0);

    if (opts.title) {
        int tx = (w - (int)strlen(opts.title)) / 2;
        if (tx < 0) tx = 0;
        ncplane_putstr_yx(dplane, 1, tx, opts.title);
    }
    if (opts.text) ncplane_putstr_yx(dplane, 3, 2, opts.text);

    int iy = 5, ix = 2, iw = w - 4;
    ncplane_putstr_yx(dplane, iy, ix, "┌");
    for (int i = 0; i < iw - 2; i++) ncplane_putstr_yx(dplane, iy, ix + 1 + i, "─");
    ncplane_putstr_yx(dplane, iy, ix + iw - 1, "┐");
    ncplane_putstr_yx(dplane, iy + 1, ix, "│");
    ncplane_putstr_yx(dplane, iy + 1, ix + iw - 1, "│");
    ncplane_putstr_yx(dplane, iy + 2, ix, "└");
    for (int i = 0; i < iw - 2; i++) ncplane_putstr_yx(dplane, iy + 2, ix + 1 + i, "─");
    ncplane_putstr_yx(dplane, iy + 2, ix + iw - 1, "┘");

    int text_x = ix + 1, text_y = iy + 1, max_len = iw - 2;
    char *buf = calloc(max_len + 1, 1);
    if (!buf) { ncplane_destroy(dplane); return 0; }
    int curpos = 0, offset = 0;

    const char *btn_ok = opts.buttons[0] ? opts.buttons[0] : "OK";
    const char *btn_cancel = opts.buttons[1] ? opts.buttons[1] : "Cancel";
    int bok_len = (int)strlen(btn_ok), bcancel_len = (int)strlen(btn_cancel);
    int btn_y = h - 3;
    int bok_x = (w - (bok_len + bcancel_len + 6)) / 2;
    int bcancel_x = bok_x + bok_len + 4;

    bool done = false, accepted = false;
    int selected_btn = 1;
    struct ncinput ni;

    while (!done) {
        ncplane_printf_yx(dplane, btn_y, bok_x - 1,
            selected_btn == 1 ? "[%s]" : " %s ", btn_ok);
        ncplane_printf_yx(dplane, btn_y, bcancel_x - 1,
            selected_btn == 2 ? "[%s]" : " %s ", btn_cancel);

        size_t buflen = strlen(buf);
        if (curpos - offset >= max_len) offset = curpos - max_len + 1;
        if (offset > curpos) offset = curpos;
        if (offset > (int)buflen) offset = (int)buflen;
        int copy_len = max_len;
        if (offset + copy_len > (int)buflen) copy_len = (int)buflen - offset;
        if (copy_len < 0) copy_len = 0;
        char visible[256];
        strncpy(visible, buf + offset, copy_len);
        visible[copy_len] = '\0';
        ncplane_printf_yx(dplane, text_y, text_x, "%-*s", max_len, visible);
        ncplane_cursor_move_yx(dplane, text_y, text_x + curpos - offset);
        notcurses_render(nc);

        notcurses_get_blocking(nc, &ni);
        if (ni.evtype == NCTYPE_RELEASE) continue;

        if (ni.id == NCKEY_ENTER || ni.id == '\n' || ni.id == '\r') {
            accepted = (selected_btn == 1); done = true;
        } else if (ni.id == NCKEY_ESC) {
            accepted = false; done = true;
        } else if (ni.id == '\t') {
            selected_btn = (selected_btn == 1) ? 2 : 1;
        } else if (ni.id == NCKEY_LEFT && curpos > 0) {
            curpos--;
        } else if (ni.id == NCKEY_RIGHT && curpos < (int)strlen(buf)) {
            curpos++;
        } else if ((ni.id == NCKEY_BACKSPACE || ni.id == 0x08 || ni.id == 0x7f) && curpos > 0) {
            memmove(buf + curpos - 1, buf + curpos, strlen(buf) - curpos + 1);
            curpos--;
        } else if (ni.id == NCKEY_HOME) {
            curpos = 0;
        } else if (ni.id == NCKEY_END) {
            curpos = (int)strlen(buf);
        } else if (isprint((unsigned char)ni.id) && (int)strlen(buf) < max_len) {
            memmove(buf + curpos + 1, buf + curpos, strlen(buf) - curpos + 1);
            buf[curpos++] = (char)ni.id;
        }
    }

    ncplane_destroy(dplane);
    notcurses_render(nc);

    int ret = 0;
    if (accepted) {
        lua_pushstring(L, buf);
        ret = 1;
        if (opts.return_button) {
            lua_pushinteger(L, selected_btn);
            ret = 2;
        }
    }
    free(buf);
    return ret;
}

/* ------------------------------------------------------------------ */
/* File browser dialog (open_dialog / save_dialog)                      */

#define FB_MAX_FILES 4096

typedef struct {
    char name[256];
    bool is_dir;
} FileEntry;

static int cmp_file_entries(const void *a, const void *b) {
    const FileEntry *ea = a, *eb = b;
    if (strcmp(ea->name, "..") == 0) return -1;
    if (strcmp(eb->name, "..") == 0) return  1;
    if (ea->is_dir != eb->is_dir) return (int)eb->is_dir - (int)ea->is_dir;
    return strcasecmp(ea->name, eb->name);
}

static int load_directory(const char *path, FileEntry *entries, int max,
                           bool only_dirs) {
    DIR *d = opendir(path);
    if (!d) return 0;
    int n = 0;
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL && n < max) {
        if (strcmp(ent->d_name, ".") == 0) continue;
        char full[PATH_MAX];
        snprintf(full, sizeof(full), "%s/%s", path, ent->d_name);
        struct stat st;
        bool is_dir = (stat(full, &st) == 0 && S_ISDIR(st.st_mode));
        if (only_dirs && !is_dir && strcmp(ent->d_name, "..") != 0) continue;
        strncpy(entries[n].name, ent->d_name, 255);
        entries[n].name[255] = '\0';
        entries[n].is_dir = is_dir;
        n++;
    }
    closedir(d);
    qsort(entries, n, sizeof(FileEntry), cmp_file_entries);
    return n;
}

/* Draw a single row with a left-to-right gradient background */
static void draw_gradient_row(struct ncplane *plane, int row, int width,
                               int r1, int g1, int b1,
                               int r2, int g2, int b2,
                               int fr, int fg, int fb) {
    for (int cx = 0; cx < width; cx++) {
        int r, g, b;
        lerp_rgb(cx, width, r1, g1, b1, r2, g2, b2, &r, &g, &b);
        ncplane_set_bg_rgb8(plane, r, g, b);
        ncplane_set_fg_rgb8(plane, fr, fg, fb);
        ncplane_putchar_yx(plane, row, cx, ' ');
    }
}

static int file_browser_dialog(DialogOptions opts, lua_State *L, bool save_mode) {
    if (!nc) return 0;

    /* Starting directory */
    char cwd[PATH_MAX];
    if (opts.dir && opts.dir[0])
        snprintf(cwd, sizeof(cwd), "%s", opts.dir);
    else if (!getcwd(cwd, sizeof(cwd)))
        snprintf(cwd, sizeof(cwd), ".");
    /* Remove trailing slash except for root */
    size_t cwl = strlen(cwd);
    if (cwl > 1 && cwd[cwl - 1] == '/') cwd[cwl - 1] = '\0';

    /* Initial filename (save mode) */
    char fname[512] = "";
    if (save_mode && opts.file)
        snprintf(fname, sizeof(fname), "%s", opts.file);
    int fname_cur = (int)strlen(fname);

    /* Dialog dimensions */
    struct ncplane *std = notcurses_stdplane(nc);
    unsigned scr_rows, scr_cols;
    ncplane_dim_yx(std, &scr_rows, &scr_cols);
    int dw = (int)scr_cols - 4;  if (dw > 88) dw = 88;  if (dw < 24) dw = 24;
    int dh = (int)scr_rows - 2;  if (dh < 14) dh = 14;
    int dy = ((int)scr_rows - dh) / 2;
    int dx = ((int)scr_cols - dw) / 2;

    struct ncplane_options dpopt = {
        .y = dy, .x = dx, .rows = dh, .cols = dw, .name = "filebrowser"
    };
    struct ncplane *dp = ncplane_create(std, &dpopt);
    if (!dp) return 0;
    ncplane_move_top(dp);

    /* Allocate file entry buffers */
    FileEntry *entries  = malloc(FB_MAX_FILES * sizeof(FileEntry));
    int       *filtered = malloc(FB_MAX_FILES * sizeof(int));
    if (!entries || !filtered) {
        free(entries); free(filtered);
        ncplane_destroy(dp); return 0;
    }

    /* Layout constants */
    int path_row  = 1;
    int filt_row  = 2;
    int sep1_row  = 3;
    int list_top  = 4;
    int list_bot  = save_mode ? dh - 5 : dh - 3; /* excl. */
    int sep2_row  = list_bot;
    int fn_row    = save_mode ? dh - 4 : -1;
    int btn_row   = dh - 2;
    int list_h    = list_bot - list_top;
    if (list_h < 1) list_h = 1;
    int entry_w   = dw - 12;  if (entry_w < 4) entry_w = 4;

    /* State */
    char filter[128] = "";
    int  filter_cur = 0;
    int  n_entries = 0, n_filtered = 0;
    int  cur_item = 0, scroll = 0;
    /* focus: 0=list 1=filter 2=fname(save) 3=ok 4=cancel */
    int  focus = 0;

    #define RELOAD() do { \
        n_entries = load_directory(cwd, entries, FB_MAX_FILES, opts.only_dirs); \
        n_filtered = 0; \
        for (int _j = 0; _j < n_entries; _j++) \
            if (!filter[0] || str_contains_ci(entries[_j].name, filter)) \
                filtered[n_filtered++] = _j; \
        if (cur_item >= n_filtered) cur_item = n_filtered > 0 ? n_filtered-1 : 0; \
        if (scroll > cur_item) scroll = cur_item; \
    } while (0)

    RELOAD();

    struct ncinput ni;
    bool done = false, accepted = false;
    char result[PATH_MAX] = "";

    while (!done) {
        /* ── Background fill ── */
        for (int row = 0; row < dh; row++) {
            for (int col = 0; col < dw; col++) {
                ncplane_set_bg_rgb8(dp, 18, 18, 24);
                ncplane_set_fg_rgb8(dp, 80, 80, 90);
                ncplane_putchar_yx(dp, row, col, ' ');
            }
        }

        /* ── Title (gradient bar) ── */
        draw_gradient_row(dp, 0, dw, 30, 60, 140, 20, 20, 24, 200, 210, 255);
        const char *title = opts.title ? opts.title : (save_mode ? "Save File" : "Open File");
        int tx = (dw - (int)strlen(title)) / 2;
        ncplane_set_fg_rgb8(dp, 240, 240, 255);
        ncplane_set_bg_rgb8(dp, 25, 50, 120);
        ncplane_putstr_yx(dp, 0, tx > 0 ? tx : 0, title);

        /* ── Current path ── */
        ncplane_set_fg_rgb8(dp, 100, 175, 255);
        ncplane_set_bg_rgb8(dp, 18, 18, 24);
        int path_avail = dw - 8;
        const char *pdisp = cwd;
        if ((int)strlen(cwd) > path_avail) pdisp = cwd + strlen(cwd) - path_avail;
        ncplane_printf_yx(dp, path_row, 1, "Path: %.*s", dw - 8, pdisp);

        /* ── Filter entry ── */
        bool filter_focused = (focus == 1);
        ncplane_set_bg_rgb8(dp, filter_focused ? 28 : 22, filter_focused ? 28 : 22,
                            filter_focused ? 38 : 30);
        ncplane_set_fg_rgb8(dp, 200, 200, 210);
        int f_off = filter_cur > entry_w ? filter_cur - entry_w : 0;
        ncplane_printf_yx(dp, filt_row, 1, "Filter:[%-*.*s]",
                          entry_w, entry_w, filter + f_off);

        /* ── Separator ── */
        ncplane_set_fg_rgb8(dp, 45, 55, 75);
        ncplane_set_bg_rgb8(dp, 18, 18, 24);
        for (int c = 0; c < dw; c++) ncplane_putchar_yx(dp, sep1_row, c, '-');

        /* ── File list ── */
        if (cur_item < scroll) scroll = cur_item;
        if (cur_item >= scroll + list_h) scroll = cur_item - list_h + 1;

        for (int i = 0; i < list_h; i++) {
            int fi = scroll + i;
            int row = list_top + i;
            if (fi >= n_filtered) {
                /* empty row */
                ncplane_set_bg_rgb8(dp, 18, 18, 24);
                for (int c = 0; c < dw; c++) ncplane_putchar_yx(dp, row, c, ' ');
                continue;
            }
            int ei = filtered[fi];
            bool is_dir = entries[ei].is_dir;
            bool sel = (fi == cur_item && focus == 0);

            if (sel) {
                /* Gradient highlight: blue */
                draw_gradient_row(dp, row, dw,
                                  35, 75, 180, 60, 110, 220,
                                  220, 235, 255);
                ncplane_set_bg_rgb8(dp, 48, 90, 195);
            } else {
                ncplane_set_bg_rgb8(dp, 18, 18, 24);
            }

            /* Icon */
            if (is_dir)
                ncplane_set_fg_rgb8(dp, sel ? 180 : 100, sel ? 220 : 160, 255);
            else
                ncplane_set_fg_rgb8(dp, sel ? 230 : 175, sel ? 235 : 175, sel ? 255 : 190);

            const char *icon = is_dir ? " [/] " : "     ";
            ncplane_putstr_yx(dp, row, 0, icon);

            /* Name, truncated */
            int maxname = dw - 6;
            ncplane_printf_yx(dp, row, 5, "%-*.*s", maxname, maxname, entries[ei].name);
        }

        /* ── Separator bottom ── */
        ncplane_set_fg_rgb8(dp, 45, 55, 75);
        ncplane_set_bg_rgb8(dp, 18, 18, 24);
        for (int c = 0; c < dw; c++) ncplane_putchar_yx(dp, sep2_row, c, '-');

        /* ── Filename entry (save mode) ── */
        if (save_mode && fn_row >= 0) {
            bool fn_focused = (focus == 2);
            ncplane_set_bg_rgb8(dp, fn_focused ? 28 : 22, fn_focused ? 28 : 22,
                                fn_focused ? 38 : 30);
            ncplane_set_fg_rgb8(dp, 210, 210, 220);
            int fn_off = fname_cur > entry_w ? fname_cur - entry_w : 0;
            ncplane_printf_yx(dp, fn_row, 1, "Name:  [%-*.*s]",
                              entry_w, entry_w, fname + fn_off);
        }

        /* ── Buttons ── */
        ncplane_set_bg_rgb8(dp, 18, 18, 24);
        if (focus == 3) draw_gradient_row(dp, btn_row, dw/2 - 1,
                                          30, 65, 160, 60, 110, 200, 220, 235, 255);
        if (focus == 4) draw_gradient_row(dp, btn_row, dw - dw/2,
                                          30, 65, 160, 60, 110, 200, 220, 235, 255);

        ncplane_set_bg_rgb8(dp, focus == 3 ? 45 : 22, focus == 3 ? 85 : 22,
                            focus == 3 ? 175 : 28);
        ncplane_set_fg_rgb8(dp, focus == 3 ? 240 : 160, focus == 3 ? 240 : 160,
                            focus == 3 ? 255 : 175);
        ncplane_printf_yx(dp, btn_row, dw/2 - 5,
                          focus == 3 ? "[ OK ]" : "  OK  ");

        ncplane_set_bg_rgb8(dp, focus == 4 ? 45 : 22, focus == 4 ? 25 : 22,
                            focus == 4 ? 25 : 28);
        ncplane_set_fg_rgb8(dp, focus == 4 ? 255 : 160, focus == 4 ? 160 : 160,
                            focus == 4 ? 160 : 175);
        ncplane_printf_yx(dp, btn_row, dw/2 + 2,
                          focus == 4 ? "[Cancel]" : " Cancel ");

        notcurses_render(nc);
        notcurses_get_blocking(nc, &ni);
        if (ni.evtype == NCTYPE_RELEASE) continue;

        uint32_t k = ni.id;

        /* ── Navigation ── */
        if (k == NCKEY_ESC) {
            done = true;
        } else if (k == '\t') {
            /* cycle: list → filter → [fname] → ok → cancel → list */
            if (!save_mode) {
                int order[] = {0, 1, 3, 4};
                for (int i = 0; i < 4; i++) {
                    if (order[i] == focus) { focus = order[(i+1)%4]; break; }
                }
            } else {
                int order[] = {0, 1, 2, 3, 4};
                for (int i = 0; i < 5; i++) {
                    if (order[i] == focus) { focus = order[(i+1)%5]; break; }
                }
            }
        } else if (k == NCKEY_UP) {
            if (focus == 0 && cur_item > 0) cur_item--;
        } else if (k == NCKEY_DOWN) {
            if (focus == 0 && cur_item < n_filtered - 1) cur_item++;
        } else if (k == NCKEY_PGUP) {
            if (focus == 0) { cur_item -= list_h - 1; if (cur_item < 0) cur_item = 0; }
        } else if (k == NCKEY_PGDOWN) {
            if (focus == 0) {
                cur_item += list_h - 1;
                if (cur_item >= n_filtered) cur_item = n_filtered > 0 ? n_filtered-1 : 0;
            }
        } else if (k == NCKEY_HOME) {
            if (focus == 0) cur_item = 0;
            else if (focus == 1) filter_cur = 0;
            else if (focus == 2) fname_cur = 0;
        } else if (k == NCKEY_END) {
            if (focus == 0) cur_item = n_filtered > 0 ? n_filtered - 1 : 0;
            else if (focus == 1) filter_cur = (int)strlen(filter);
            else if (focus == 2) fname_cur = (int)strlen(fname);
        } else if (k == NCKEY_BACKSPACE || k == 0x08 || k == 0x7f) {
            if (focus == 1 && filter_cur > 0) {
                memmove(filter + filter_cur - 1, filter + filter_cur,
                        strlen(filter) - filter_cur + 1);
                filter_cur--;
                RELOAD();
            } else if (focus == 2 && fname_cur > 0) {
                memmove(fname + fname_cur - 1, fname + fname_cur,
                        strlen(fname) - fname_cur + 1);
                fname_cur--;
            } else if (focus == 0) {
                /* Go to parent directory */
                char *slash = strrchr(cwd, '/');
                if (slash && slash > cwd) { *slash = '\0'; RELOAD(); }
            }
        } else if (k == NCKEY_ENTER || k == '\r' || k == '\n') {
            if (focus == 4) {
                done = true; /* Cancel */
            } else if (focus == 3) {
                /* OK */
                if (save_mode && fname[0]) {
                    snprintf(result, sizeof(result), "%s/%s", cwd, fname);
                    accepted = true; done = true;
                } else if (!save_mode && n_filtered > 0) {
                    int ei = filtered[cur_item];
                    if (!entries[ei].is_dir || opts.only_dirs) {
                        snprintf(result, sizeof(result), "%s/%s", cwd, entries[ei].name);
                        accepted = true; done = true;
                    }
                }
            } else if (focus == 0) {
                if (n_filtered > 0) {
                    int ei = filtered[cur_item];
                    if (entries[ei].is_dir) {
                        /* Navigate into directory */
                        if (strcmp(entries[ei].name, "..") == 0) {
                            char *slash = strrchr(cwd, '/');
                            if (slash && slash > cwd) *slash = '\0';
                        } else {
                            size_t cl = strlen(cwd);
                            snprintf(cwd + cl, sizeof(cwd) - cl, "/%s", entries[ei].name);
                        }
                        filter[0] = '\0'; filter_cur = 0;
                        RELOAD();
                    } else {
                        if (save_mode) {
                            /* Pre-fill filename entry with selected file */
                            snprintf(fname, sizeof(fname), "%s", entries[ei].name);
                            fname_cur = (int)strlen(fname);
                            focus = 2;
                        } else {
                            snprintf(result, sizeof(result), "%s/%s", cwd, entries[ei].name);
                            accepted = true; done = true;
                        }
                    }
                }
            }
        } else if (k >= 32 && k <= 126) {
            /* Printable character */
            if (focus == 1 && (int)strlen(filter) < (int)sizeof(filter) - 1) {
                memmove(filter + filter_cur + 1, filter + filter_cur,
                        strlen(filter) - filter_cur + 1);
                filter[filter_cur++] = (char)k;
                cur_item = 0; scroll = 0;
                RELOAD();
            } else if (focus == 2 && (int)strlen(fname) < (int)sizeof(fname) - 1) {
                memmove(fname + fname_cur + 1, fname + fname_cur,
                        strlen(fname) - fname_cur + 1);
                fname[fname_cur++] = (char)k;
            } else if (focus == 0) {
                /* Type first letter to jump to matching entry */
                for (int j = 0; j < n_filtered; j++) {
                    if (tolower((unsigned char)entries[filtered[j]].name[0]) ==
                        tolower((unsigned char)k)) {
                        cur_item = j; break;
                    }
                }
            }
        }
    }
    #undef RELOAD

    free(entries);
    free(filtered);
    ncplane_destroy(dp);
    notcurses_render(nc);

    if (!accepted || !result[0]) return 0;

    if (opts.multiple) {
        lua_createtable(L, 1, 0);
        lua_pushstring(L, result);
        lua_rawseti(L, -2, 1);
    } else {
        lua_pushstring(L, result);
    }
    return 1;
}

int open_dialog(DialogOptions opts, lua_State *L) {
    if (!opts.title) opts.title = "Open File";
    return file_browser_dialog(opts, L, false);
}

int save_dialog(DialogOptions opts, lua_State *L) {
    if (!opts.title) opts.title = "Save File";
    return file_browser_dialog(opts, L, true);
}

int progress_dialog(DialogOptions opts, lua_State *L,
    bool (*work)(void (*update)(double percent, const char *text, void *userdata), void *userdata)) {
    if (!nc) return 0;
    struct ncplane *std = notcurses_stdplane(nc);
    unsigned rows, cols;
    ncplane_dim_yx(std, &rows, &cols);
    int h = 10, w = (int)cols * 2 / 3;
    if (w > 60) w = 60;
    int y = ((int)rows - h) / 2, x = ((int)cols - w) / 2;

    struct ncplane_options popt = {
        .y = y, .x = x, .rows = h, .cols = w, .name = "progress",
    };
    struct ncplane *dplane = ncplane_create(std, &popt);
    if (!dplane) return 0;
    ncplane_set_base(dplane, " ", 0, 0);

    if (opts.title) {
        int tx = (w - (int)strlen(opts.title)) / 2;
        if (tx < 0) tx = 0;
        ncplane_putstr_yx(dplane, 1, tx, opts.title);
    }
    if (opts.text) ncplane_putstr_yx(dplane, 3, 2, opts.text);

    int bar_y = 5, bar_x = 2, bar_w = w - 4;
    ncplane_putstr_yx(dplane, bar_y, bar_x, "[");
    ncplane_putstr_yx(dplane, bar_y, bar_x + bar_w - 1, "]");
    for (int i = 1; i < bar_w - 1; i++)
        ncplane_putstr_yx(dplane, bar_y, bar_x + i, " ");

    bool cancelled = false;
    struct ncinput ni;

    void update(double percent, const char *text, void *udata) {
        (void)udata;
        int filled = (int)((bar_w - 2) * percent / 100.0);
        if (filled < 0) filled = 0;
        if (filled > bar_w - 2) filled = bar_w - 2;
        for (int i = 1; i < bar_w - 1; i++)
            ncplane_putstr_yx(dplane, bar_y, bar_x + i, i <= filled ? "=" : " ");
        char line[256];
        snprintf(line, sizeof(line), "%.0f%% %s", percent, text ? text : "");
        ncplane_printf_yx(dplane, bar_y + 2, bar_x, "%-*s", bar_w, line);
        notcurses_render(nc);
        uint32_t k;
        while ((k = notcurses_get_nblock(nc, &ni)) != 0 && k != (uint32_t)-1)
            if (ni.evtype != NCTYPE_RELEASE && ni.id == NCKEY_ESC)
                { cancelled = true; break; }
    }

    work(update, NULL);
    ncplane_destroy(dplane);
    notcurses_render(nc);

    if (cancelled) { lua_pushboolean(L, true); return 1; }
    return 0;
}

/* ------------------------------------------------------------------ */
/* list_dialog — scrollable filtered list (Bug F)                        */

/* Case-insensitive substring search */
static bool str_contains_ci(const char *hay, const char *needle) {
    if (!needle || !*needle) return true;
    size_t nl = strlen(needle);
    size_t hl = strlen(hay);
    for (size_t i = 0; i + nl <= hl; i++) {
        if (strncasecmp(hay + i, needle, nl) == 0) return true;
    }
    return false;
}

/* Returns true if the row at row_idx passes all filter words */
static bool row_matches(lua_State *L, int items_idx, int num_cols, int row_idx,
                        int search_col, const char *filter) {
    if (!filter || !*filter) return true;
    /* search_col is 1-based index within a row; row_idx is 0-based row number */
    int item_idx = row_idx * num_cols + (search_col - 1) + 1; /* 1-based Lua index */
    lua_rawgeti(L, items_idx, item_idx);
    const char *cell = lua_tostring(L, -1);
    if (!cell) cell = "";
    bool ok = str_contains_ci(cell, filter);
    lua_pop(L, 1);
    return ok;
}

int list_dialog(DialogOptions opts, lua_State *L) {
    if (!nc) return 0;
    struct ncplane *std = notcurses_stdplane(nc);
    unsigned scr_rows, scr_cols;
    ncplane_dim_yx(std, &scr_rows, &scr_cols);

    int num_cols = opts.columns ? (int)lua_rawlen(L, opts.columns) : 1;
    int num_items = (int)lua_rawlen(L, opts.items);
    int num_rows = (num_items + num_cols - 1) / num_cols;

    /* Dialog dimensions */
    int dw = (int)scr_cols - 4;
    if (dw > 80) dw = 80;
    int dh = (int)scr_rows - 4;
    if (dh < 8) dh = 8;
    int dy = ((int)scr_rows - dh) / 2;
    int dx = ((int)scr_cols - dw) / 2;

    struct ncplane_options dpopt = {
        .y = dy, .x = dx, .rows = dh, .cols = dw, .name = "listdlg"
    };
    struct ncplane *dp = ncplane_create(std, &dpopt);
    if (!dp) return 0;
    ncplane_set_base(dp, " ", 0, 0);
    ncplane_move_top(dp);

    /* Layout:
     * Row 0: title
     * Row 1: filter entry [___________]
     * Row 2..dh-4: item list
     * Row dh-3: column headers (if columns provided)
     * Row dh-2: button row
     * Row dh-1: empty */
    int list_top = opts.columns ? 3 : 2;
    int list_bot = dh - 3; /* exclusive */
    int list_h = list_bot - list_top;
    if (list_h < 1) list_h = 1;

    /* Column widths */
    int *col_w = calloc(num_cols, sizeof(int));
    for (int c = 0; c < num_cols; c++) {
        if (opts.columns) {
            lua_rawgeti(L, opts.columns, c + 1);
            int w = (int)strlen(lua_tostring(L, -1) ? lua_tostring(L, -1) : "");
            lua_pop(L, 1);
            col_w[c] = w;
        }
        for (int r = 0; r < num_rows; r++) {
            int idx = r * num_cols + c + 1;
            if (idx > num_items) break;
            lua_rawgeti(L, opts.items, idx);
            const char *s = lua_tostring(L, -1);
            int w = s ? (int)strlen(s) : 0;
            if (w > col_w[c]) col_w[c] = w;
            lua_pop(L, 1);
        }
        /* Cap column width */
        int avail = (dw - 2 - num_cols) / num_cols;
        if (col_w[c] > avail) col_w[c] = avail;
    }

    /* Collect row strings for display */
    char **row_strs = calloc(num_rows, sizeof(char *));
    for (int r = 0; r < num_rows; r++) {
        size_t len = 0;
        for (int c = 0; c < num_cols; c++) len += col_w[c] + 2;
        len += 1;
        char *s = calloc(len, 1);
        char *p = s;
        for (int c = 0; c < num_cols; c++) {
            int idx = r * num_cols + c + 1;
            const char *cell = "";
            if (idx <= num_items) {
                lua_rawgeti(L, opts.items, idx);
                cell = lua_tostring(L, -1);
                if (!cell) cell = "";
            }
            int clen = (int)strlen(cell);
            memcpy(p, cell, clen < col_w[c] ? clen : col_w[c]);
            p += clen < col_w[c] ? clen : col_w[c];
            for (int i = clen; i < col_w[c]; i++) *p++ = ' ';
            *p++ = ' ';
            if (idx <= num_items) lua_pop(L, 1);
        }
        *p = '\0';
        row_strs[r] = s;
    }

    /* Filter state */
    char filter[128] = "";
    if (opts.text) snprintf(filter, sizeof(filter), "%s", opts.text);
    int filter_cur = (int)strlen(filter);

    /* Filtered indices */
    int *filtered = calloc(num_rows, sizeof(int));
    int num_filtered = 0;

    /* Rebuild filter */
    #define REFILTER() do { \
        num_filtered = 0; \
        for (int _r = 0; _r < num_rows; _r++) \
            if (row_matches(L, opts.items, num_cols, _r, \
                            opts.search_column > 0 ? opts.search_column : 1, filter)) \
                filtered[num_filtered++] = _r; \
    } while (0)

    REFILTER();

    int cur_row = 0; /* index into filtered[] */
    if (opts.select > 0 && opts.select <= num_filtered) cur_row = opts.select - 1;
    int scroll = 0; /* first visible row in filtered[] */

    /* Buttons */
    const char *b0 = opts.buttons[0] ? opts.buttons[0] : "OK";
    const char *b1 = opts.buttons[1] ? opts.buttons[1] : "Cancel";
    int cur_btn = -1; /* -1=in list, 0=OK, 1=Cancel */

    struct ncinput ni;
    bool done = false;
    int result_row = 0; /* 1-based original index */
    int result_btn = 0;

    while (!done) {
        ncplane_erase(dp);

        /* Title */
        if (opts.title) {
            int tx = (dw - (int)strlen(opts.title)) / 2;
            ncplane_putstr_yx(dp, 0, tx > 0 ? tx : 0, opts.title);
        }

        /* Filter entry */
        ncplane_printf_yx(dp, 1, 1, "Filter: [%-*s]", dw - 12, filter);

        /* Column headers */
        if (opts.columns) {
            int hx = 1;
            for (int c = 0; c < num_cols; c++) {
                lua_rawgeti(L, opts.columns, c + 1);
                const char *hdr = lua_tostring(L, -1);
                ncplane_printf_yx(dp, 2, hx, "%-*s ", col_w[c],
                                  hdr ? hdr : "");
                lua_pop(L, 1);
                hx += col_w[c] + 1;
            }
        }

        /* Item list */
        if (num_filtered == 0) {
            ncplane_putstr_yx(dp, list_top, 2, "(no matches)");
        } else {
            if (cur_row < scroll) scroll = cur_row;
            if (cur_row >= scroll + list_h) scroll = cur_row - list_h + 1;
            for (int i = 0; i < list_h && scroll + i < num_filtered; i++) {
                int ri = filtered[scroll + i];
                bool sel = (scroll + i == cur_row) && cur_btn < 0;
                ncplane_printf_yx(dp, list_top + i, 1, sel ? ">%-*s" : " %-*s",
                                  dw - 3, row_strs[ri]);
            }
        }

        /* Buttons */
        int btn_y = dh - 2;
        int b0x = dw / 2 - (int)strlen(b0) - 3;
        int b1x = dw / 2 + 1;
        ncplane_printf_yx(dp, btn_y, b0x, cur_btn == 0 ? "[%s]" : " %s ", b0);
        ncplane_printf_yx(dp, btn_y, b1x, cur_btn == 1 ? "[%s]" : " %s ", b1);

        notcurses_render(nc);
        notcurses_get_blocking(nc, &ni);
        if (ni.evtype == NCTYPE_RELEASE) continue;

        uint32_t k = ni.id;
        if (k == NCKEY_ESC) {
            result_btn = 0; done = true;
        } else if (k == NCKEY_ENTER || k == '\r' || k == '\n') {
            if (cur_btn == 1) {
                result_btn = 0; done = true;
            } else {
                if (num_filtered > 0) result_row = filtered[cur_row] + 1;
                result_btn = cur_btn == 0 ? 1 : 1;
                done = true;
            }
        } else if (k == '\t') {
            if (cur_btn < 0) cur_btn = 0;
            else if (cur_btn == 0) cur_btn = 1;
            else cur_btn = -1;
        } else if (k == NCKEY_UP) {
            if (cur_btn < 0 && cur_row > 0) cur_row--;
        } else if (k == NCKEY_DOWN) {
            if (cur_btn < 0 && cur_row < num_filtered - 1) cur_row++;
        } else if (k == NCKEY_PGUP) {
            if (cur_btn < 0) cur_row -= list_h - 1;
            if (cur_row < 0) cur_row = 0;
        } else if (k == NCKEY_PGDOWN) {
            if (cur_btn < 0) cur_row += list_h - 1;
            if (cur_row >= num_filtered) cur_row = num_filtered > 0 ? num_filtered - 1 : 0;
        } else if (k == NCKEY_HOME) {
            if (cur_btn < 0) cur_row = 0;
            else filter_cur = 0;
        } else if (k == NCKEY_END) {
            if (cur_btn < 0) cur_row = num_filtered > 0 ? num_filtered - 1 : 0;
            else filter_cur = (int)strlen(filter);
        } else if ((k == NCKEY_BACKSPACE || k == 0x08 || k == 0x7f) && filter_cur > 0) {
            memmove(filter + filter_cur - 1, filter + filter_cur,
                    strlen(filter) - filter_cur + 1);
            filter_cur--;
            REFILTER();
            cur_row = 0; scroll = 0;
        } else if (k >= 32 && k <= 126 && (int)strlen(filter) < (int)sizeof(filter) - 1) {
            /* Type to filter */
            memmove(filter + filter_cur + 1, filter + filter_cur,
                    strlen(filter) - filter_cur + 1);
            filter[filter_cur++] = (char)k;
            REFILTER();
            cur_row = 0; scroll = 0;
        }
    }
    #undef REFILTER

    ncplane_destroy(dp);
    notcurses_render(nc);

    for (int r = 0; r < num_rows; r++) free(row_strs[r]);
    free(row_strs);
    free(filtered);
    free(col_w);

    if (result_btn == 0 || result_row == 0) return 0;

    if (opts.multiple) {
        lua_createtable(L, 1, 0);
        lua_pushinteger(L, result_row);
        lua_rawseti(L, -2, 1);
    } else {
        lua_pushinteger(L, result_row);
    }
    if (opts.return_button) {
        lua_pushinteger(L, result_btn);
        return 2;
    }
    return 1;
}

/* ------------------------------------------------------------------ */
/* spawn — POSIX fork/exec/pipe (Bug G)                                  */

/* Parse a command string into argv. Returns argc; argv must be freed with
 * free_argv(). Each element is strdup'd. */
static int parse_cmd(const char *cmd, char ***argv_out) {
    int argc = 0, cap = 16;
    char **argv = malloc(cap * sizeof(char *));
    const char *p = cmd;
    while (*p) {
        while (*p == ' ') p++;
        if (!*p) break;
        /* Build one token */
        char tok[4096];
        int tlen = 0;
        while (*p && *p != ' ') {
            if (*p == '"' || *p == '\'') {
                char q = *p++;
                while (*p && *p != q) {
                    if (*p == '\\' && *(p+1)) p++;
                    if (tlen < (int)sizeof(tok) - 1) tok[tlen++] = *p;
                    p++;
                }
                if (*p) p++; /* closing quote */
            } else {
                if (tlen < (int)sizeof(tok) - 1) tok[tlen++] = *p;
                p++;
            }
        }
        tok[tlen] = '\0';
        if (argc >= cap) {
            cap *= 2;
            argv = realloc(argv, cap * sizeof(char *));
        }
        argv[argc++] = strdup(tok);
    }
    argv = realloc(argv, (argc + 1) * sizeof(char *));
    argv[argc] = NULL;
    *argv_out = argv;
    return argc;
}

static void free_argv(char **argv, int argc) {
    for (int i = 0; i < argc; i++) free(argv[i]);
    free(argv);
}

bool spawn(lua_State *L, Process *proc, int index, const char *cmd, const char *cwd, int envi,
    bool monitor_stdout, bool monitor_stderr, const char **error) {
    NProcess *np = (NProcess *)proc;
    memset(np, 0, sizeof(NProcess));
    np->pid = -1;
    np->stdin_fd = np->stdout_fd = np->stderr_fd = -1;

    /* Parse command */
    char **argv = NULL;
    int argc = parse_cmd(cmd, &argv);
    if (argc == 0) {
        free_argv(argv, argc);
        if (error) *error = "empty command";
        return false;
    }

    /* Build envp if requested */
    int envc = 0;
    char **envp = NULL;
    if (envi) {
        envc = (int)lua_rawlen(L, envi);
        envp = malloc((envc + 1) * sizeof(char *));
        for (int i = 0; i < envc; i++) {
            lua_rawgeti(L, envi, i + 1);
            const char *s = lua_tostring(L, -1);
            envp[i] = s ? strdup(s) : strdup("");
            lua_pop(L, 1);
        }
        envp[envc] = NULL;
    }

    /* Create pipes: [0]=read end, [1]=write end */
    int p_stdin[2] = {-1, -1}, p_stdout[2] = {-1, -1}, p_stderr[2] = {-1, -1};
    if (pipe(p_stdin) < 0 || pipe(p_stdout) < 0 || pipe(p_stderr) < 0) {
        if (error) *error = strerror(errno);
        close(p_stdin[0]);  close(p_stdin[1]);
        close(p_stdout[0]); close(p_stdout[1]);
        close(p_stderr[0]); close(p_stderr[1]);
        free_argv(argv, argc);
        if (envp) { for (int i = 0; i < envc; i++) free(envp[i]); free(envp); }
        return false;
    }

    pid_t pid = fork();
    if (pid < 0) {
        if (error) *error = strerror(errno);
        close(p_stdin[0]);  close(p_stdin[1]);
        close(p_stdout[0]); close(p_stdout[1]);
        close(p_stderr[0]); close(p_stderr[1]);
        free_argv(argv, argc);
        if (envp) { for (int i = 0; i < envc; i++) free(envp[i]); free(envp); }
        return false;
    }

    if (pid == 0) {
        /* Child */
        dup2(p_stdin[0],  STDIN_FILENO);
        dup2(p_stdout[1], STDOUT_FILENO);
        dup2(p_stderr[1], STDERR_FILENO);
        close(p_stdin[0]);  close(p_stdin[1]);
        close(p_stdout[0]); close(p_stdout[1]);
        close(p_stderr[0]); close(p_stderr[1]);

        if (cwd) { if (chdir(cwd) < 0) {} }

        if (envp) {
            extern char **environ;
            environ = envp;
        }
        execvp(argv[0], argv);
        _exit(127);
    }

    /* Parent */
    close(p_stdin[0]);
    close(p_stdout[1]);
    close(p_stderr[1]);
    free_argv(argv, argc);
    if (envp) { for (int i = 0; i < envc; i++) free(envp[i]); free(envp); }

    np->pid = pid;
    np->stdin_fd = p_stdin[1];
    np->stdout_fd = p_stdout[0];
    np->stderr_fd = p_stderr[0];
    np->running = true;
    np->monitor_stdout = monitor_stdout;
    np->monitor_stderr = monitor_stderr;

    /* Set non-blocking on read ends for the monitoring loop */
    fcntl(np->stdout_fd, F_SETFL, O_NONBLOCK);
    fcntl(np->stderr_fd, F_SETFL, O_NONBLOCK);

    /* Add to active process list */
    np->next = active_processes;
    active_processes = np;

    /* Register in Lua registry to prevent GC */
    lua_checkstack(L, 3);
    luaL_getsubtable(L, LUA_REGISTRYINDEX, "spawn_procs");
    lua_pushvalue(L, index);
    lua_pushboolean(L, 1);
    lua_settable(L, -3);
    lua_pop(L, 1); /* pop spawn_procs */

    return true;
}

size_t process_size(void) { return sizeof(NProcess); }

bool is_process_running(Process *proc) {
    NProcess *np = (NProcess *)proc;
    return np && np->running;
}

void wait_process(Process *proc) {
    NProcess *np = (NProcess *)proc;
    if (!np || !np->running) return;
    int status;
    waitpid(np->pid, &status, 0);
    np->running = false;
    np->exit_status = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

char *read_process_output(Process *proc, char option, size_t *len, const char **error, int *code) {
    NProcess *np = (NProcess *)proc;
    if (!np || np->stdout_fd < 0) {
        if (len) *len = 0;
        if (error) *error = NULL; /* EOF */
        return NULL;
    }

    int fd = np->stdout_fd;
    /* Temporarily set blocking for synchronous read */
    int flags = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);

    char *result = NULL;
    *len = 0;

    if (option == 'n') {
        /* Read exactly *len bytes */
        size_t want = *len;
        result = malloc(want + 1);
        ssize_t n = read(fd, result, want);
        if (n > 0) {
            *len = (size_t)n;
            result[n] = '\0';
        } else {
            free(result);
            result = NULL;
            *len = 0;
            if (n < 0 && error) *error = strerror(errno);
            if (code && n < 0) *code = errno;
        }
    } else if (option == 'a') {
        /* Read until EOF */
        size_t cap = 4096;
        result = malloc(cap);
        ssize_t total = 0, n;
        while ((n = read(fd, result + total, cap - total - 1)) > 0) {
            total += n;
            if ((size_t)total >= cap - 1) {
                cap *= 2;
                result = realloc(result, cap);
            }
        }
        result[total] = '\0';
        *len = total;
        if (total == 0) { free(result); result = NULL; }
    } else {
        /* 'l' or 'L': read one line */
        size_t cap = 256;
        result = malloc(cap);
        ssize_t total = 0;
        char ch;
        ssize_t n;
        while ((n = read(fd, &ch, 1)) > 0) {
            if ((size_t)total >= cap - 1) {
                cap *= 2;
                result = realloc(result, cap);
            }
            if (ch == '\n') {
                if (option == 'L') result[total++] = ch;
                break;
            }
            if (ch != '\r') result[total++] = ch;
        }
        result[total] = '\0';
        *len = total;
        if (n <= 0 && total == 0) { free(result); result = NULL; }
    }

    fcntl(fd, F_SETFL, flags); /* restore non-blocking */
    return result;
}

void write_process_input(Process *proc, const char *s, size_t len) {
    NProcess *np = (NProcess *)proc;
    if (!np || np->stdin_fd < 0) return;
    ssize_t written = 0;
    while ((size_t)written < len) {
        ssize_t n = write(np->stdin_fd, s + written, len - written);
        if (n <= 0) break;
        written += n;
    }
}

void close_process_input(Process *proc) {
    NProcess *np = (NProcess *)proc;
    if (np && np->stdin_fd != -1) { close(np->stdin_fd); np->stdin_fd = -1; }
}

void kill_process(Process *proc, int sig) {
    NProcess *np = (NProcess *)proc;
    if (!np || np->pid <= 0) return;
    kill(np->pid, sig ? sig : SIGTERM);
}

int get_process_exit_status(Process *proc) {
    NProcess *np = (NProcess *)proc;
    return np ? np->exit_status : -1;
}

void cleanup_process(Process *proc) {
    NProcess *np = (NProcess *)proc;
    if (!np) return;
    /* Remove from active list */
    NProcess **pp = &active_processes;
    while (*pp) {
        if (*pp == np) { *pp = np->next; break; }
        pp = &(*pp)->next;
    }
    close_process_input(np);
    if (np->stdout_fd != -1) { close(np->stdout_fd); np->stdout_fd = -1; }
    if (np->stderr_fd != -1) { close(np->stderr_fd); np->stderr_fd = -1; }
}

void suspend(void) {}
void quit(void)    { want_quit = true; }
