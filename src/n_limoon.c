/*
 * n_limoon.c - Notcurses frontend for Li Moon.
 *
 * Implements the platform-dependent functions required by Li Moon
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
#include "limoon.h"
#include "limoon_platform.h"

/* ------------------------------------------------------------------ */
/* UI Theme                                                              */

typedef struct {
    /* Tab bar */
    uint32_t tab_act_a, tab_act_b, tab_act_text;   /* active tab gradient + text */
    uint32_t tab_ina_a, tab_ina_b, tab_ina_text;   /* inactive tab gradient + text */
    uint32_t tab_close_act, tab_close_ina;          /* × button colors */
    uint32_t tabbar_bg, tabbar_sep;                 /* empty space + separator */
    /* Status bar */
    uint32_t sb_bg, sb_fg, sb_sep;
    /* Scrollbar */
    uint32_t sc_bg, sc_arrow, sc_thumb, sc_track;
    /* Split bar */
    uint32_t sp_fg, sp_bg;
    /* Find bar */
    uint32_t fb_bar, fb_entry, fb_border, fb_label;
    uint32_t fb_text, fb_cursor, fb_cursor_fg, fb_dim;
    uint32_t fb_btn, fb_nav, fb_hiall, fb_close, fb_repl;
    uint32_t fb_opt_on, fb_opt_off;
    /* Dialogs (message, input, list) */
    uint32_t dlg_bg, dlg_border, dlg_title, dlg_text;
    uint32_t dlg_btn_fg, dlg_btn_bg, dlg_foc_fg, dlg_foc_bg, dlg_accel;
    /* File browser dialog */
    uint32_t fd_bg, fd_title_a, fd_title_b, fd_title_fg, fd_title_bg;
    uint32_t fd_path, fd_sep;
    uint32_t fd_sel_a, fd_sel_b, fd_sel_bg;
    uint32_t fd_dir_sel, fd_dir_unsel, fd_file_sel, fd_file_unsel;
    uint32_t fd_entry_focus, fd_entry_blur, fd_entry_text;
    uint32_t fd_ok_foc_bg, fd_ok_foc_fg, fd_cancel_foc_bg, fd_cancel_foc_fg;
    uint32_t fd_idle_bg, fd_idle_fg;
    /* File tree panel */
    uint32_t ft_bg, ft_title_fg, ft_title_bg;
    uint32_t ft_dir_fg, ft_file_fg;
    uint32_t ft_sel_fg, ft_sel_bg;
    uint32_t ft_sep_fg, ft_sep_bg;
    uint32_t ft_icon_fg;
} LimoonTheme;

/* Helpers: unpack 0xRRGGBB and set fg/bg on a plane */
#define TH_FG(p,c) ncplane_set_fg_rgb8(p,((c)>>16)&0xFF,((c)>>8)&0xFF,(c)&0xFF)
#define TH_BG(p,c) ncplane_set_bg_rgb8(p,((c)>>16)&0xFF,((c)>>8)&0xFF,(c)&0xFF)

/* Blue (original Catppuccin-based palette) */
static const LimoonTheme THEME_BLUE = {
    .tab_act_a=0x1E41AF, .tab_act_b=0x4B78E6, .tab_act_text=0xDCE6FF,
    .tab_ina_a=0x1C1C20, .tab_ina_b=0x34343C, .tab_ina_text=0x91919B,
    .tab_close_act=0xFF7878, .tab_close_ina=0xB45A5A,
    .tabbar_bg=0x141418, .tabbar_sep=0x3C3C46,
    .sb_bg=0x1E1E24, .sb_fg=0xC8C8D2, .sb_sep=0x50505A,
    .sc_bg=0x1E1E28, .sc_arrow=0xB4B4C8, .sc_thumb=0x8C8CB4, .sc_track=0x373746,
    .sp_fg=0x646464, .sp_bg=0x282828,
    .fb_bar=0x1E1E2E, .fb_entry=0x11111B, .fb_border=0x89B4FA, .fb_label=0x6C7086,
    .fb_text=0xCDD6F4, .fb_cursor=0x89B4FA, .fb_cursor_fg=0x1E1E2E, .fb_dim=0x45475A,
    .fb_btn=0x313244, .fb_nav=0x89B4FA, .fb_hiall=0xF9E2AF, .fb_close=0xF38BA8,
    .fb_repl=0xA6E3A1, .fb_opt_on=0xA6E3A1, .fb_opt_off=0x585B70,
    .dlg_bg=0x313244, .dlg_border=0x89B4FA, .dlg_title=0xCDD6F4, .dlg_text=0xBAC2DE,
    .dlg_btn_fg=0xCDD6F4, .dlg_btn_bg=0x45475A, .dlg_foc_fg=0x1E1E2E,
    .dlg_foc_bg=0x89B4FA, .dlg_accel=0xF9E2AF,
    .fd_bg=0x121218, .fd_title_a=0x1E3C8C, .fd_title_b=0x141418,
    .fd_title_fg=0xF0F0FF, .fd_title_bg=0x193278,
    .fd_path=0x64AFFF, .fd_sep=0x2D3748,
    .fd_sel_a=0x234BB4, .fd_sel_b=0x3C6EDC, .fd_sel_bg=0x305AC3,
    .fd_dir_sel=0xB4DCFF, .fd_dir_unsel=0x64A0FF,
    .fd_file_sel=0xE6EBFF, .fd_file_unsel=0xAFAFBE,
    .fd_entry_focus=0x1C1C26, .fd_entry_blur=0x16161E, .fd_entry_text=0xD2D2DC,
    .fd_ok_foc_bg=0x2D55AF, .fd_ok_foc_fg=0xF0F0FF,
    .fd_cancel_foc_bg=0x2D1919, .fd_cancel_foc_fg=0xFFA0A0,
    .fd_idle_bg=0x16161C, .fd_idle_fg=0xA0A0AF,
    .ft_bg=0x1A1A24, .ft_title_fg=0xDCE6FF, .ft_title_bg=0x1E3C8C,
    .ft_dir_fg=0x89B4FA, .ft_file_fg=0xBAC2DE,
    .ft_sel_fg=0xF0F4FF, .ft_sel_bg=0x2E4E9A,
    .ft_sep_fg=0x3C3C5A, .ft_sep_bg=0x141418,
    .ft_icon_fg=0x6C7086,
};

/* Black & White */
static const LimoonTheme THEME_BW = {
    .tab_act_a=0x2A2A2A, .tab_act_b=0x484848, .tab_act_text=0xFFFFFF,
    .tab_ina_a=0x141414, .tab_ina_b=0x202020, .tab_ina_text=0x666666,
    .tab_close_act=0xFFAAAA, .tab_close_ina=0x886666,
    .tabbar_bg=0x0A0A0A, .tabbar_sep=0x2A2A2A,
    .sb_bg=0x141414, .sb_fg=0xCCCCCC, .sb_sep=0x444444,
    .sc_bg=0x0F0F0F, .sc_arrow=0xAAAAAA, .sc_thumb=0x888888, .sc_track=0x333333,
    .sp_fg=0x888888, .sp_bg=0x1A1A1A,
    .fb_bar=0x1A1A1A, .fb_entry=0x0D0D0D, .fb_border=0xAAAAAA, .fb_label=0x666666,
    .fb_text=0xDDDDDD, .fb_cursor=0xAAAAAA, .fb_cursor_fg=0x0A0A0A, .fb_dim=0x444444,
    .fb_btn=0x2A2A2A, .fb_nav=0xCCCCCC, .fb_hiall=0xEEEEEE, .fb_close=0xFFAAAA,
    .fb_repl=0xBBBBBB, .fb_opt_on=0xCCCCCC, .fb_opt_off=0x555555,
    .dlg_bg=0x2A2A2A, .dlg_border=0x888888, .dlg_title=0xEEEEEE, .dlg_text=0xCCCCCC,
    .dlg_btn_fg=0xEEEEEE, .dlg_btn_bg=0x3A3A3A, .dlg_foc_fg=0x0A0A0A,
    .dlg_foc_bg=0xAAAAAA, .dlg_accel=0xFFFFAA,
    .fd_bg=0x111111, .fd_title_a=0x2A2A2A, .fd_title_b=0x111111,
    .fd_title_fg=0xFFFFFF, .fd_title_bg=0x222222,
    .fd_path=0xAAAAAA, .fd_sep=0x333333,
    .fd_sel_a=0x3A3A3A, .fd_sel_b=0x505050, .fd_sel_bg=0x404040,
    .fd_dir_sel=0xFFFFFF, .fd_dir_unsel=0xAAAAAA,
    .fd_file_sel=0xFFFFFF, .fd_file_unsel=0x777777,
    .fd_entry_focus=0x1E1E1E, .fd_entry_blur=0x161616, .fd_entry_text=0xDDDDDD,
    .fd_ok_foc_bg=0x555555, .fd_ok_foc_fg=0xFFFFFF,
    .fd_cancel_foc_bg=0x3A1E1E, .fd_cancel_foc_fg=0xFFAAAA,
    .fd_idle_bg=0x1A1A1A, .fd_idle_fg=0xAAAAAA,
    .ft_bg=0x181818, .ft_title_fg=0xFFFFFF, .ft_title_bg=0x2A2A2A,
    .ft_dir_fg=0xCCCCCC, .ft_file_fg=0x888888,
    .ft_sel_fg=0xFFFFFF, .ft_sel_bg=0x404040,
    .ft_sep_fg=0x333333, .ft_sep_bg=0x101010,
    .ft_icon_fg=0x666666,
};

/* Green — limão (default) */
static const LimoonTheme THEME_GREEN = {
    .tab_act_a=0x0F4A20, .tab_act_b=0x1E8A3E, .tab_act_text=0xC0EBC8,
    .tab_ina_a=0x0C160E, .tab_ina_b=0x182A1A, .tab_ina_text=0x4A7A54,
    .tab_close_act=0xFF9090, .tab_close_ina=0x8A5050,
    .tabbar_bg=0x080E09, .tabbar_sep=0x1A2A1C,
    .sb_bg=0x0F1910, .sb_fg=0x80C888, .sb_sep=0x2A4A2E,
    .sc_bg=0x0A140C, .sc_arrow=0x5AAA68, .sc_thumb=0x2E7A3C, .sc_track=0x1A2E1C,
    .sp_fg=0x2A5E30, .sp_bg=0x0A140C,
    .fb_bar=0x0F1910, .fb_entry=0x080E09, .fb_border=0x3DBA5E, .fb_label=0x3A5E40,
    .fb_text=0xC0EBC8, .fb_cursor=0x3DBA5E, .fb_cursor_fg=0x080E09, .fb_dim=0x1E3A22,
    .fb_btn=0x152218, .fb_nav=0x3DBA5E, .fb_hiall=0xD8D870, .fb_close=0xF07878,
    .fb_repl=0x5ADA7A, .fb_opt_on=0x5ADA7A, .fb_opt_off=0x304A36,
    .dlg_bg=0x152218, .dlg_border=0x3DBA5E, .dlg_title=0xC0EBC8, .dlg_text=0x90BE98,
    .dlg_btn_fg=0xC0EBC8, .dlg_btn_bg=0x1E3222, .dlg_foc_fg=0x080E09,
    .dlg_foc_bg=0x3DBA5E, .dlg_accel=0xD8D870,
    .fd_bg=0x0A140C, .fd_title_a=0x0E3C1A, .fd_title_b=0x0A140C,
    .fd_title_fg=0xC0EBC8, .fd_title_bg=0x0C2E16,
    .fd_path=0x3DBA5E, .fd_sep=0x1E3A22,
    .fd_sel_a=0x0E4A1E, .fd_sel_b=0x1A7A36, .fd_sel_bg=0x145028,
    .fd_dir_sel=0xB0E8B8, .fd_dir_unsel=0x3DA050,
    .fd_file_sel=0xD0EDD4, .fd_file_unsel=0x607060,
    .fd_entry_focus=0x0E1E10, .fd_entry_blur=0x0A140C, .fd_entry_text=0xC0EBC8,
    .fd_ok_foc_bg=0x1A6830, .fd_ok_foc_fg=0xC0EBC8,
    .fd_cancel_foc_bg=0x3A1A1A, .fd_cancel_foc_fg=0xFFA0A0,
    .fd_idle_bg=0x0C1A0E, .fd_idle_fg=0x608060,
    .ft_bg=0x0A140C, .ft_title_fg=0xC0EBC8, .ft_title_bg=0x0E3C1A,
    .ft_dir_fg=0x3DBA5E, .ft_file_fg=0x80A885,
    .ft_sel_fg=0xC0EBC8, .ft_sel_bg=0x0E4A1E,
    .ft_sep_fg=0x1A2E1C, .ft_sep_bg=0x080E09,
    .ft_icon_fg=0x3A5E40,
};

static const LimoonTheme *T = &THEME_GREEN; /* active theme */

/* Li Moon globals */
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

/* ------------------------------------------------------------------ */
/* Pane tree (split view support)                                       */

typedef enum { NC_SINGLE, NC_VSPLIT, NC_HSPLIT } NCPaneType;

typedef struct NCPane {
    NCPaneType type;
    int y, x, rows, cols, split_pos;
    struct ncplane *split_plane;    /* split bar plane (non-SINGLE)  */
    struct ncplane *scrollbar_plane;/* vertical scrollbar (SINGLE only) */
    SciObject *view;                /* Scintilla view (SINGLE only)  */
    struct NCPane *child1, *child2;
} NCPane;

/* Global Notcurses context */
static struct notcurses *nc = NULL;
static View *current_view = NULL;
static NCPane *root_pane = NULL;

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
static bool tabs_shown = false;

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
static int utf8_visual_width(const char *s);
static size_t utf8_truncate(const char *src, char *out, size_t out_size, int max_cells);
static int draw_utf8(struct ncplane *plane, int y, int x, const char *s, int max_cells);
static int utf8_next(const char *s, int pos);
static int utf8_prev(const char *s, int pos);
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

/* Find/replace history (move-to-front, most recent at index 0) */
#define FIND_HIST_MAX 20
static char find_hist[FIND_HIST_MAX][256];
static int  find_hist_n = 0;
static char repl_hist[FIND_HIST_MAX][256];
static int  repl_hist_n = 0;

/* Command entry state */
static bool command_entry_active = false;
static char command_entry_label[256] = "";
static int command_entry_height_stored = 1;
static SciObject *saved_focused_view = NULL;

/* Statusbar state */
static bool statusbar_visible = true;
static char statusbar_text0[256] = "";
static char statusbar_text1[256] = "";

/* Mouse state (Bug B) */
static int mouse_pressed_button = 0;

/* Scrollbar state */
static bool scrollbar_enabled = false;

/* Emergency exit */
static bool want_quit = false;
static int ctrl_c_count = 0;
static double last_ctrl_c_time = 0.0;

/* Dirty flag: only call notcurses_render() when something actually changed. */
static bool needs_render = true;

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
/* File tree panel                                                       */

#define FT_MAX 4096
#define FT_DEFAULT_WIDTH 30

typedef struct {
    char path[4096];    /* PATH_MAX equivalent */
    char name[256];
    bool is_dir;
    bool expanded;
    int depth;
} FTEntry;

static FTEntry ft_entries[FT_MAX];
static int ft_count = 0;
static char ft_root_path[4096] = "";
static int ft_cursor = 0;
static int ft_scroll = 0;
static bool ft_visible = false;
static bool ft_focused = false;
static int ft_width = FT_DEFAULT_WIDTH;
static struct ncplane *ft_plane = NULL;

static int ft_compare(const void *a, const void *b) {
    const FTEntry *x = (const FTEntry *)a;
    const FTEntry *y = (const FTEntry *)b;
    if (x->is_dir && !y->is_dir) return -1;
    if (!x->is_dir && y->is_dir) return 1;
    return strcasecmp(x->name, y->name);
}

/* ---- Scan arena: single heap buffer, reused across directory scans ---- */
typedef struct { char *buf; size_t cap, used; } FTArena;
static FTArena ft_arena = {0};

static FTEntry *ft_arena_push(void) {
    size_t need = ft_arena.used + sizeof(FTEntry);
    if (need > ft_arena.cap) {
        size_t new_cap = ft_arena.cap ? ft_arena.cap * 2 : 64 * sizeof(FTEntry);
        while (new_cap < need) new_cap *= 2;
        char *nb = realloc(ft_arena.buf, new_cap);
        if (!nb) return NULL;
        ft_arena.buf = nb;
        ft_arena.cap = new_cap;
    }
    FTEntry *e = (FTEntry *)(ft_arena.buf + ft_arena.used);
    ft_arena.used += sizeof(FTEntry);
    return e;
}

static void ft_arena_reset(void) { ft_arena.used = 0; }

static void ft_scan_into(const char *dir_path, int depth, int insert_at) {
    ft_arena_reset();
    DIR *d = opendir(dir_path);
    if (!d) return;
    struct dirent *de;
    int n = 0;
    while ((de = readdir(d)) != NULL) {
        if (de->d_name[0] == '.') continue;
        FTEntry *e = ft_arena_push();
        if (!e) break;
        snprintf(e->path, 4096, "%s/%s", dir_path, de->d_name);
        snprintf(e->name, 256, "%s", de->d_name);
        e->depth = depth;
        e->expanded = false;
        struct stat st;
        e->is_dir = (stat(e->path, &st) == 0 && S_ISDIR(st.st_mode));
        n++;
    }
    closedir(d);
    if (n == 0) return;
    FTEntry *tmp = (FTEntry *)ft_arena.buf;
    qsort(tmp, n, sizeof(FTEntry), ft_compare);
    int after = ft_count - insert_at;
    if (ft_count + n > FT_MAX) n = FT_MAX - ft_count;
    if (n > 0) {
        memmove(&ft_entries[insert_at + n], &ft_entries[insert_at],
                after * sizeof(FTEntry));
        memcpy(&ft_entries[insert_at], tmp, (size_t)n * sizeof(FTEntry));
        ft_count += n;
    }
    /* arena stays allocated — reset reuses it next scan */
}

static void ft_load_root(const char *path) {
    if (path) {
        strncpy(ft_root_path, path, 4095);
        ft_root_path[4095] = '\0';
    } else {
        ft_root_path[0] = '\0';
    }
    ft_count = 0; ft_cursor = 0; ft_scroll = 0;
    if (!path || !path[0]) return;
    ft_scan_into(path, 0, 0);
}

static void ft_expand(int i) {
    if (i < 0 || i >= ft_count) return;
    if (!ft_entries[i].is_dir || ft_entries[i].expanded) return;
    ft_scan_into(ft_entries[i].path, ft_entries[i].depth + 1, i + 1);
    ft_entries[i].expanded = true;
}

static void ft_collapse(int i) {
    if (i < 0 || i >= ft_count) return;
    if (!ft_entries[i].is_dir || !ft_entries[i].expanded) return;
    int base = ft_entries[i].depth;
    int j = i + 1;
    while (j < ft_count && ft_entries[j].depth > base) j++;
    int remove_n = j - (i + 1);
    if (remove_n > 0) {
        memmove(&ft_entries[i + 1], &ft_entries[j],
                (ft_count - j) * sizeof(FTEntry));
        ft_count -= remove_n;
        if (ft_cursor > i && ft_cursor < j) ft_cursor = i;
        else if (ft_cursor >= j) ft_cursor -= remove_n;
    }
    ft_entries[i].expanded = false;
    if (ft_scroll >= ft_count) ft_scroll = ft_count > 0 ? ft_count - 1 : 0;
}

static void ft_open_file(const char *path, bool do_split) {
    if (!lua) return;
    if (do_split) {
        lua_getglobal(lua, "view");
        if (lua_istable(lua, -1)) {
            lua_getfield(lua, -1, "split");
            if (lua_isfunction(lua, -1)) {
                lua_insert(lua, -2);
                lua_pushboolean(lua, 1);
                if (lua_pcall(lua, 2, 0, 0) != LUA_OK) lua_pop(lua, 1);
            } else { lua_pop(lua, 2); }
        } else { lua_pop(lua, 1); }
    }
    lua_getglobal(lua, "io");
    if (lua_istable(lua, -1)) {
        lua_getfield(lua, -1, "open_file");
        lua_remove(lua, -2);
        if (lua_isfunction(lua, -1)) {
            lua_pushstring(lua, path);
            if (lua_pcall(lua, 1, 0, 0) != LUA_OK) lua_pop(lua, 1);
        } else { lua_pop(lua, 1); }
    } else { lua_pop(lua, 1); }
    ft_focused = false;
    if (focused_view) scintilla_set_focus(focused_view, true);
    needs_render = true;
}

static void ft_draw(void) {
    if (!ft_plane || !ft_visible) return;
    unsigned rows, cols;
    ncplane_dim_yx(ft_plane, &rows, &cols);
    ncplane_erase(ft_plane);

    /* Title bar */
    TH_FG(ft_plane, T->ft_title_fg);
    TH_BG(ft_plane, T->ft_title_bg);
    int content_w = (int)cols - 1; /* last col = separator */
    char raw_title[256], title[256];
    const char *rname = strrchr(ft_root_path, '/');
    snprintf(raw_title, sizeof(raw_title), " %s", rname ? rname + 1 : ft_root_path);
    for (int c = 0; c < content_w; c++) ncplane_putchar_yx(ft_plane, 0, c, ' ');
    utf8_truncate(raw_title, title, sizeof(title), content_w > 0 ? content_w : 0);
    draw_utf8(ft_plane, 0, 0, title, content_w > 0 ? content_w : 0);

    /* Separator column (rightmost) */
    TH_FG(ft_plane, T->ft_sep_fg);
    TH_BG(ft_plane, T->ft_sep_bg);
    for (int r = 0; r < (int)rows; r++)
        ncplane_putstr_yx(ft_plane, r, (int)cols - 1, "│");

    /* Entries */
    int vis = (int)rows - 1;
    for (int r = 0; r < vis; r++) {
        int i = ft_scroll + r;
        if (i >= ft_count) break;
        FTEntry *e = &ft_entries[i];
        bool is_cursor = (i == ft_cursor);
        bool is_sel = is_cursor && ft_focused;

        /* Row background */
        TH_BG(ft_plane, is_sel ? T->ft_sel_bg : T->ft_bg);
        TH_FG(ft_plane, is_sel ? T->ft_sel_fg :
              (e->is_dir ? T->ft_dir_fg : T->ft_file_fg));
        /* Clear row */
        for (int c = 0; c < content_w; c++)
            ncplane_putchar_yx(ft_plane, r + 1, c, ' ');

        /* Cursor indicator (unfocused) */
        if (is_cursor && !ft_focused) {
            TH_FG(ft_plane, T->ft_icon_fg);
            TH_BG(ft_plane, T->ft_bg);
        }

        int col = e->depth * 2;
        if (col >= content_w - 3) col = content_w - 3;
        if (col < 0) col = 0;

        /* Icon */
        TH_FG(ft_plane, is_sel ? T->ft_sel_fg : T->ft_icon_fg);
        TH_BG(ft_plane, is_sel ? T->ft_sel_bg : T->ft_bg);
        if (e->is_dir) {
            ncplane_putstr_yx(ft_plane, r + 1, col,
                              e->expanded ? "▾" : "▸");
        } else {
            ncplane_putchar_yx(ft_plane, r + 1, col, ' ');
        }

        /* Name */
        TH_FG(ft_plane, is_sel ? T->ft_sel_fg :
              (e->is_dir ? T->ft_dir_fg : T->ft_file_fg));
        TH_BG(ft_plane, is_sel ? T->ft_sel_bg : T->ft_bg);
        int name_col = col + 2;
        int max_ch = content_w - name_col;
        if (max_ch > 0) {
            /* Cell-based (not byte-based) truncation: strncpy() by byte
             * count against a cell budget under- or over-shoots as soon as
             * the name contains any non-ASCII character. */
            char trunc[256];
            int name_w = utf8_visual_width(e->name);
            int budget = e->is_dir ? max_ch - 1 : max_ch; /* reserve 1 cell for trailing / */
            if (budget < 0) budget = 0;
            utf8_truncate(e->name, trunc, sizeof(trunc), budget);
            /* Append '/' to dirs, only if the name wasn't truncated */
            if (e->is_dir && name_w <= budget) {
                size_t tlen = strlen(trunc);
                if (tlen + 1 < sizeof(trunc)) {
                    trunc[tlen] = '/'; trunc[tlen + 1] = '\0';
                }
            }
            draw_utf8(ft_plane, r + 1, name_col, trunc, max_ch);
        }
    }
}

static void ft_toggle(void);   /* forward declaration */
static void handle_resize(void); /* forward declaration for ft_toggle */
static inline int view_top_row(void);
static inline unsigned view_overhead(void);

static bool ft_handle_key(int key, int mods) {
    if (!ft_visible || !ft_focused) return false;
    (void)mods;

    unsigned rows = 0;
    if (ft_plane) ncplane_dim_yx(ft_plane, &rows, NULL);
    int vis = rows > 1 ? (int)rows - 1 : 1;

    switch (key) {
        case SCK_UP:
            if (ft_cursor > 0) ft_cursor--;
            if (ft_cursor < ft_scroll) ft_scroll = ft_cursor;
            break;
        case SCK_DOWN:
            if (ft_cursor < ft_count - 1) ft_cursor++;
            if (ft_cursor >= ft_scroll + vis) ft_scroll = ft_cursor - vis + 1;
            break;
        case SCK_PRIOR: /* Page Up */
            ft_cursor -= vis;
            if (ft_cursor < 0) ft_cursor = 0;
            ft_scroll -= vis;
            if (ft_scroll < 0) ft_scroll = 0;
            break;
        case SCK_NEXT: /* Page Down */
            ft_cursor += vis;
            if (ft_cursor >= ft_count) ft_cursor = ft_count > 0 ? ft_count - 1 : 0;
            if (ft_cursor >= ft_scroll + vis) ft_scroll = ft_cursor - vis + 1;
            break;
        case SCK_RIGHT: /* Right: expand dir */
            if (ft_cursor >= 0 && ft_cursor < ft_count) {
                FTEntry *e = &ft_entries[ft_cursor];
                if (e->is_dir && !e->expanded) ft_expand(ft_cursor);
                else if (!e->is_dir) { ft_open_file(e->path, false); return true; }
            }
            break;
        case SCK_LEFT: /* Left: collapse dir (or go to parent) */
            if (ft_cursor >= 0 && ft_cursor < ft_count) {
                FTEntry *e = &ft_entries[ft_cursor];
                if (e->is_dir && e->expanded) {
                    ft_collapse(ft_cursor);
                } else if (e->depth > 0) {
                    /* Move cursor to parent */
                    int pdepth = e->depth - 1;
                    int pi = ft_cursor - 1;
                    while (pi >= 0 && ft_entries[pi].depth != pdepth) pi--;
                    if (pi >= 0) {
                        ft_cursor = pi;
                        if (ft_cursor < ft_scroll) ft_scroll = ft_cursor;
                    }
                }
            }
            break;
        case '\r': /* Enter */
            if (ft_cursor >= 0 && ft_cursor < ft_count) {
                FTEntry *e = &ft_entries[ft_cursor];
                if (e->is_dir) {
                    if (e->expanded) ft_collapse(ft_cursor);
                    else ft_expand(ft_cursor);
                } else {
                    ft_open_file(e->path, false);
                    return true;
                }
            }
            break;
        case '\t': /* Tab: open in split */
            if (ft_cursor >= 0 && ft_cursor < ft_count) {
                FTEntry *e = &ft_entries[ft_cursor];
                if (!e->is_dir) {
                    ft_open_file(e->path, true);
                    return true;
                }
            }
            break;
        case SCK_ESCAPE:
            ft_focused = false;
            if (focused_view) scintilla_set_focus(focused_view, true);
            break;
        default:
            return false;
    }
    needs_render = true;
    return true;
}

static void ft_toggle(void) {
    if (ft_visible) {
        ft_visible = false;
        ft_focused = false;
        if (ft_plane) { ncplane_destroy(ft_plane); ft_plane = NULL; }
        if (focused_view) scintilla_set_focus(focused_view, true);
    } else {
        char root[4096] = "";
        if (lua) {
            lua_getglobal(lua, "limoon");
            if (lua_istable(lua, -1)) {
                lua_getfield(lua, -1, "workspace");
                if (lua_istable(lua, -1)) {
                    lua_getfield(lua, -1, "dir");
                    if (lua_isstring(lua, -1)) {
                        const char *d = lua_tostring(lua, -1);
                        if (d && d[0]) {
                            strncpy(root, d, 4095);
                            root[4095] = '\0';
                        }
                    }
                    lua_pop(lua, 1); /* dir */
                }
                lua_pop(lua, 1); /* workspace */
            }
            lua_pop(lua, 1); /* limoon */
        }
        if (!root[0]) {
            if (!getcwd(root, 4096)) root[0] = '\0';
        }
        ft_load_root(root);
        ft_visible = true;
        ft_focused = true;
        if (nc) {
            unsigned rows, cols;
            ncplane_dim_yx(notcurses_stdplane(nc), &rows, &cols);
            unsigned over = view_overhead();
            int vh = (int)(rows > over ? rows - over : 1);
            if (find_visible && vh > 3) vh -= 3;
            if (command_entry_active && command_entry_height_stored > 0)
                vh = vh > command_entry_height_stored
                    ? vh - command_entry_height_stored : 1;
            struct ncplane_options opt = {
                .y = view_top_row(), .x = 0,
                .rows = (unsigned)vh,
                .cols = (unsigned)ft_width,
                .name = "filetree"
            };
            ft_plane = ncplane_create(notcurses_stdplane(nc), &opt);
            if (!ft_plane) { ft_visible = false; ft_focused = false; return; }
        }
    }
    handle_resize();
    needs_render = true;
}

/* Focus the file tree (if visible). Returns true if focused. */
bool ft_focus(void) {
    if (!ft_visible) return false;
    ft_focused = true;
    if (focused_view) scintilla_set_focus(focused_view, false);
    needs_render = true;
    return true;
}

/* ------------------------------------------------------------------ */
/* NCPane helper functions                                              */

static NCPane *ncpane_new(SciObject *view) {
    NCPane *p = calloc(1, sizeof(NCPane));
    if (!p) return NULL;
    p->type = NC_SINGLE;
    p->view = view;
    return p;
}

/* Recursively resize/reposition a pane and all its children. */
static void ncpane_resize(NCPane *pane, int rows, int cols, int y, int x) {
    if (!pane || rows < 1 || cols < 1) return;
    /* Protecao adicional contra divisao por zero */
    if (pane->cols < 1) pane->cols = cols;
    if (pane->rows < 1) pane->rows = rows;
    if (pane->type == NC_VSPLIT) {
        int ssize = pane->cols > 0
            ? (int)((double)pane->split_pos * cols / pane->cols)
            : cols / 2;
        if (ssize < 1) ssize = 1;
        if (ssize >= cols - 1) ssize = cols - 2;
        pane->split_pos = ssize;
        ncpane_resize(pane->child1, rows, ssize, y, x);
        ncpane_resize(pane->child2, rows, cols - ssize - 1, y, x + ssize + 1);
        if (pane->split_plane) {
            ncplane_resize_simple(pane->split_plane, (unsigned)rows, 1);
            ncplane_move_yx(pane->split_plane, y, x + ssize);
        }
    } else if (pane->type == NC_HSPLIT) {
        int ssize = pane->rows > 0
            ? (int)((double)pane->split_pos * rows / pane->rows)
            : rows / 2;
        if (ssize < 1) ssize = 1;
        if (ssize >= rows - 1) ssize = rows - 2;
        pane->split_pos = ssize;
        ncpane_resize(pane->child1, ssize, cols, y, x);
        ncpane_resize(pane->child2, rows - ssize - 1, cols, y + ssize + 1, x);
        if (pane->split_plane) {
            ncplane_resize_simple(pane->split_plane, 1, (unsigned)cols);
            ncplane_move_yx(pane->split_plane, y + ssize, x);
        }
    } else {
        int sci_cols = (scrollbar_enabled && cols > 1) ? cols - 1 : cols;
        struct ncplane *p = scintilla_get_plane(pane->view);
        if (p) {
            ncplane_resize_simple(p, (unsigned)rows, (unsigned)sci_cols);
            ncplane_move_yx(p, y, x);
        }
        scintilla_resize(pane->view);
        /* Create or resize scrollbar plane */
        if (scrollbar_enabled && cols > 1) {
            if (!pane->scrollbar_plane) {
                struct ncplane_options sopt = {
                    .y = y, .x = x + sci_cols,
                    .rows = (unsigned)rows, .cols = 1, .name = "scrollbar"
                };
                pane->scrollbar_plane = ncplane_create(notcurses_stdplane(nc), &sopt);
            } else {
                ncplane_resize_simple(pane->scrollbar_plane, (unsigned)rows, 1);
                ncplane_move_yx(pane->scrollbar_plane, y, x + sci_cols);
            }
        } else if (pane->scrollbar_plane) {
            ncplane_destroy(pane->scrollbar_plane);
            pane->scrollbar_plane = NULL;
        }
    }
    pane->y = y; pane->x = x; pane->rows = rows; pane->cols = cols;
}

/* Draw the vertical scrollbar for a single-view pane. */
static void draw_scrollbar(NCPane *pane) {
    if (!pane || !pane->scrollbar_plane || !pane->view) return;
    struct ncplane *sp = pane->scrollbar_plane;
    int rows = pane->rows;
    if (rows < 2) return;

    intptr_t first_line  = SS(pane->view, SCI_GETFIRSTVISIBLELINE, 0, 0);
    intptr_t total_lines = SS(pane->view, SCI_GETLINECOUNT, 0, 0);
    int visible_rows = rows;

    int track_h = rows - 2; /* exclude top/bottom arrows */
    if (track_h < 1) track_h = 1;

    int thumb_h = (total_lines > 0)
        ? (int)((double)track_h * visible_rows / total_lines)
        : track_h;
    if (thumb_h < 1) thumb_h = 1;
    if (thumb_h > track_h) thumb_h = track_h;

    int max_scroll = (int)(total_lines - visible_rows);
    if (max_scroll < 0) max_scroll = 0;
    int thumb_pos = (max_scroll > 0)
        ? (int)((double)(track_h - thumb_h) * first_line / max_scroll)
        : 0;
    if (thumb_pos < 0) thumb_pos = 0;
    if (thumb_pos + thumb_h > track_h) thumb_pos = track_h - thumb_h;

    ncplane_erase(sp);
    ncplane_set_styles(sp, NCSTYLE_NONE);

    /* Top arrow */
    TH_FG(sp, T->sc_arrow); TH_BG(sp, T->sc_bg);
    ncplane_putstr_yx(sp, 0, 0, "▲");

    /* Track and thumb */
    for (int r = 0; r < track_h; r++) {
        bool in_thumb = (r >= thumb_pos && r < thumb_pos + thumb_h);
        if (in_thumb) {
            TH_FG(sp, T->sc_thumb); TH_BG(sp, T->sc_bg);
            ncplane_putstr_yx(sp, r + 1, 0, "█");
        } else {
            TH_FG(sp, T->sc_track); TH_BG(sp, T->sc_bg);
            ncplane_putstr_yx(sp, r + 1, 0, "│");
        }
    }

    /* Bottom arrow */
    TH_FG(sp, T->sc_arrow); TH_BG(sp, T->sc_bg);
    ncplane_putstr_yx(sp, rows - 1, 0, "▼");
}

/* Find the leaf pane containing screen position (y, x). */
static NCPane *ncpane_find_at(NCPane *pane, int y, int x) {
    if (!pane) return NULL;
    /* Early out: coordenadas fora deste pane */
    if (y < pane->y || y >= pane->y + pane->rows ||
        x < pane->x || x >= pane->x + pane->cols)
        return NULL;
    if (pane->type == NC_SINGLE) {
        return pane;
    }
    NCPane *f = ncpane_find_at(pane->child1, y, x);
    return f ? f : ncpane_find_at(pane->child2, y, x);
}

/* Render all views and split bars. */
static void ncpane_render(NCPane *pane) {
    if (!pane) return;
    if (pane->type == NC_SINGLE) {
        if (pane->view) {
            scintilla_render(pane->view);
            if (scrollbar_enabled) draw_scrollbar(pane);
        }
    } else {
        if (pane->split_plane) {
            TH_FG(pane->split_plane, T->sp_fg);
            TH_BG(pane->split_plane, T->sp_bg);
            ncplane_set_styles(pane->split_plane, NCSTYLE_NONE);
            if (pane->type == NC_VSPLIT) {
                for (int r = 0; r < pane->rows; r++)
                    ncplane_putstr_yx(pane->split_plane, r, 0, "│");
            } else {
                for (int c = 0; c < pane->cols; c++)
                    ncplane_putstr_yx(pane->split_plane, 0, c, "─");
            }
        }
        ncpane_render(pane->child1);
        ncpane_render(pane->child2);
    }
}

/* Update cursors for all views in the pane tree. */
static void ncpane_update_cursors(NCPane *pane) {
    if (!pane) return;
    if (pane->type == NC_SINGLE) {
        if (pane->view && pane->view != focused_view)
            scintilla_update_cursor(pane->view);
    } else {
        ncpane_update_cursors(pane->child1);
        ncpane_update_cursors(pane->child2);
    }
}

/* Return the parent pane of child (searched by view pointer or NCPane pointer). */
static NCPane *ncpane_get_parent(NCPane *root, void *child) {
    if (!root || root->type == NC_SINGLE || !root->child1 || !root->child2) return NULL;
    if ((void *)root->child1->view == child || (void *)root->child2->view == child ||
        (void *)root->child1 == child || (void *)root->child2 == child)
        return root;
    NCPane *p = ncpane_get_parent(root->child1, child);
    if (p) return p;
    return ncpane_get_parent(root->child2, child);
}

/* Free a pane subtree, calling delete_view for each leaf view. */
static void ncpane_free(NCPane *pane, void (*delete_view)(SciObject *)) {
    if (!pane) return;
    if (pane->type == NC_SINGLE) {
        if (delete_view && pane->view) delete_view(pane->view);
        if (pane->scrollbar_plane) {
            ncplane_destroy(pane->scrollbar_plane);
            pane->scrollbar_plane = NULL;
        }
    } else {
        ncpane_free(pane->child1, delete_view);
        ncpane_free(pane->child2, delete_view);
        if (pane->split_plane) {
            ncplane_destroy(pane->split_plane);
            pane->split_plane = NULL;
        }
    }
    free(pane);
}

/* ------------------------------------------------------------------ */
/* Layout helpers — centralize view geometry calculations               */

/* First row of the main view (0 = tabbar hidden, 1 = shown). */
static inline int view_top_row(void) { return tabs_shown ? 1 : 0; }

/* Rows consumed by chrome (tabbar + statusbar). */
static inline unsigned view_overhead(void) {
    return (tabs_shown ? 1u : 0u) + (statusbar_visible ? 1u : 0u);
}

/* First column of the main editor area (0 = no file tree, ft_width = tree shown). */
static inline int view_left_col(void) { return ft_visible ? ft_width : 0; }
/* Width of the main editor area. */
static inline int view_edit_cols(int total_cols) {
    return ft_visible ? total_cols - ft_width : total_cols;
}

/* Row index of the status bar (-1 = off screen when not visible). */
static inline int statusbar_row(unsigned rows) {
    return statusbar_visible ? (int)rows - 1 : (int)rows;
}

/* ------------------------------------------------------------------ */

/* Find the leaf NCPane containing a given Scintilla view. */
static NCPane *ncpane_find_sci(NCPane *pane, SciObject *sci) {
    if (!pane) return NULL;
    if (pane->type == NC_SINGLE) return (pane->view == sci) ? pane : NULL;
    NCPane *f = ncpane_find_sci(pane->child1, sci);
    return f ? f : ncpane_find_sci(pane->child2, sci);
}

/* Centralized resize handler: refreshes display and resizes all panes. */
static void handle_resize(void) {
    if (!nc) return;
    needs_render = true;
    notcurses_refresh(nc, NULL, NULL);
    unsigned rows, cols;
    ncplane_dim_yx(notcurses_stdplane(nc), &rows, &cols);

    if (tabbar_plane)
        ncplane_resize_simple(tabbar_plane, 1, cols);
    if (statusbar_plane) {
        ncplane_resize_simple(statusbar_plane, 1, cols);
        ncplane_move_yx(statusbar_plane, (int)rows - 1, 0);
    }
    /* File tree plane */
    if (ft_visible && ft_plane) {
        unsigned over = view_overhead();
        int vh = (int)(rows > over ? rows - over : 1);
        if (find_visible && vh > 3) vh -= 3;
        if (command_entry_active && command_entry_height_stored > 0)
            vh = vh > command_entry_height_stored ? vh - command_entry_height_stored : 1;
        ncplane_resize_simple(ft_plane, (unsigned)vh, (unsigned)ft_width);
        ncplane_move_yx(ft_plane, view_top_row(), 0);
    }

    if (root_pane) {
        unsigned over = view_overhead();
        unsigned view_h = rows > over ? rows - over : 1;
        if (find_visible)
            view_h = view_h > 3 ? view_h - 3 : 1;
        if (command_entry_active) {
            unsigned ce_h = command_entry_height_stored > 0 ?
                            (unsigned)command_entry_height_stored : 1;
            view_h = view_h > ce_h ? view_h - ce_h : 1;
        }
        int ed_x = view_left_col();
        int ed_w = view_edit_cols((int)cols);
        if (ed_w < 1) ed_w = 1;
        ncpane_resize(root_pane, (int)view_h, ed_w, view_top_row(), ed_x);
    }
    if (command_entry_active && command_entry) {
        unsigned ce_h = command_entry_height_stored > 0 ?
                        (unsigned)command_entry_height_stored : 1;
        struct ncplane *ce_p = scintilla_get_plane(command_entry);
        if (ce_p) {
            int ce_y = statusbar_row(rows) - (int)ce_h;
            if (ce_y < 1) ce_y = 1;
            ncplane_resize_simple(ce_p, ce_h, cols);
            ncplane_move_yx(ce_p, ce_y, 0);
            scintilla_resize(command_entry);
        }
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
            needs_render = true;
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
        if (monitoring) {
            needs_render = true;
            process_output((Process *)np, buf, (size_t)n, is_stdout);
        }
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
    if (nc_mods & NCKEY_MOD_META)  sci_mods |= SCMOD_ALT;  /* Meta treated as Alt */
    return sci_mods;
}

/* ------------------------------------------------------------------ */
/* Key handling                                                          */

static void handle_keypress(struct ncinput *ni) {
    if (!focused_view) return;
    uint32_t key = ni->id;
    unsigned nc_mods = ni->modifiers;
    int sci_mods = nc_to_sci_mods(nc_mods);

    /* Normalize Kitty-protocol Shift+printable: terminals such as Kitty,
     * Alacritty and foot report Shift+a as id='a'+SHIFT and Shift+1 as
     * id='1'+SHIFT instead of id='A' / id='!'.  notcurses fills eff_text[0]
     * with the actual produced codepoint (e.g. '!' for Shift+1, 'A' for
     * Shift+a).  Li Moon expects the shifted codepoint with no SHIFT modifier
     * for printable chars (keys.lua strips SHIFT for code >= 32).  Use
     * eff_text[0] when it is a single printable codepoint and no Ctrl/Alt. */
    if ((nc_mods & NCKEY_MOD_SHIFT) &&
        !(nc_mods & (NCKEY_MOD_CTRL | NCKEY_MOD_ALT)) &&
        !nckey_synthesized_p(key) &&
        ni->eff_text[0] >= 0x20 && ni->eff_text[1] == 0) {
        key = ni->eff_text[0];
        nc_mods &= ~(unsigned)NCKEY_MOD_SHIFT;
        sci_mods &= ~SCMOD_SHIFT;
    }

    int emit_key = 0;
    /* Set when a non-Kitty control code (1–26) was remapped to letter+CTRL.
     * In that case scintilla_send_key must receive the letter + NCKEY_MOD_CTRL
     * instead of the raw code, or scinterm will misread 8→BS, 9→Tab, 13→CR. */
    bool remapped_ctrl = false;

    /* Key code to send to scinterm (NotCurses codes) vs Lua/Scintilla (SCK_* codes).
     * scintilla_send_key expects NotCurses key codes (NCKEY_*), not Scintilla codes.
     * Lua and the file tree handler use Scintilla key codes (SCK_*). */
    int scinterm_key = (int)key;

    if (key >= 1 && key <= 26) {
        /* Control codes 1–26: may arrive with or without NCKEY_MOD_CTRL.
         * Without CTRL flag (non-Kitty): 0x08/0x09/0x0a/0x0d are ambiguous
         * (Backspace/Tab/Enter sent as raw bytes, NOT Ctrl+H/I/J/M).
         * With CTRL flag (Kitty or some terminals): always Ctrl+letter. */
        if (!(nc_mods & NCKEY_MOD_CTRL)) {
            switch (key) {
                case 0x08: emit_key = SCK_BACK; scinterm_key = NCKEY_BACKSPACE; break;
                case 0x09: emit_key = '\t';     scinterm_key = '\t';             break;
                case 0x0a: /* fallthrough */
                case 0x0d: emit_key = '\r';     scinterm_key = NCKEY_ENTER;      break;
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
            case NCKEY_UP:        emit_key = SCK_UP;      scinterm_key = NCKEY_UP;     break;
            case NCKEY_DOWN:      emit_key = SCK_DOWN;    scinterm_key = NCKEY_DOWN;   break;
            case NCKEY_LEFT:      emit_key = SCK_LEFT;    scinterm_key = NCKEY_LEFT;   break;
            case NCKEY_RIGHT:     emit_key = SCK_RIGHT;   scinterm_key = NCKEY_RIGHT;  break;
            case NCKEY_HOME:      emit_key = SCK_HOME;    scinterm_key = NCKEY_HOME;   break;
            case NCKEY_END:       emit_key = SCK_END;     scinterm_key = NCKEY_END;    break;
            case NCKEY_PGUP:      emit_key = SCK_PRIOR;   scinterm_key = NCKEY_PGUP;   break;
            case NCKEY_PGDOWN:    emit_key = SCK_NEXT;    scinterm_key = NCKEY_PGDOWN; break;
            case NCKEY_DEL:       emit_key = SCK_DELETE;  scinterm_key = NCKEY_DEL;    break;
            case NCKEY_INS:       emit_key = SCK_INSERT;  scinterm_key = NCKEY_INS;    break;
            case NCKEY_BACKSPACE: case 0x08: case 0x7f:
                                  emit_key = SCK_BACK;    scinterm_key = NCKEY_BACKSPACE; break;
            case NCKEY_ESC:       emit_key = SCK_ESCAPE;  scinterm_key = NCKEY_ESC;    break;
            case NCKEY_ENTER: case '\r': case '\n':
                                  emit_key = '\r';        scinterm_key = NCKEY_ENTER;  break;
            case '\t':            emit_key = '\t';        scinterm_key = '\t';         break;
            /* Ctrl+especiais fora do range 1-26 */
            case 0x00: emit_key = '@';  sci_mods |= SCMOD_CTRL; break;
            case 0x1c: emit_key = '\\'; sci_mods |= SCMOD_CTRL; break;
            case 0x1d: emit_key = ']';  sci_mods |= SCMOD_CTRL; break;
            case 0x1e: emit_key = '^';  sci_mods |= SCMOD_CTRL; break;
            case 0x1f: emit_key = '_';  sci_mods |= SCMOD_CTRL; break;
            /* Function keys: map to GDK keysyms already in Li Moon's KEYSYMS table */
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

    /* Emergency exit — 3 consecutive Ctrl+C presses within a short window.
     * Ctrl+C is also the copy shortcut, so without the time window a user
     * who copies three unrelated fragments in one session would trigger
     * this and lose unsaved work. Only presses within 500ms of each other
     * count as "consecutive"; a slower cadence is normal copying, not a
     * panic mash. */
    if (emit_key == 'c' && (sci_mods & SCMOD_CTRL)) {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        double now = ts.tv_sec + ts.tv_nsec / 1e9;
        if (now - last_ctrl_c_time > 0.5) ctrl_c_count = 0;
        last_ctrl_c_time = now;
        if (++ctrl_c_count >= 3) { want_quit = true; return; }
    } else {
        ctrl_c_count = 0;
    }

    /* Kitty protocol envia Ctrl+letra como uppercase. Normalizar para lowercase.
     * Também normalizar Alt+letra para garantir que Alt+Q seja 'q', não 'Q'. */
    if ((sci_mods & (SCMOD_CTRL | SCMOD_ALT)) && emit_key >= 'A' && emit_key <= 'Z')
        emit_key += 'a' - 'A';

    /* ctrl+k toggles file tree (intercepted before Lua) */
    if (emit_key == 'k' && (sci_mods & SCMOD_CTRL) &&
        !(sci_mods & (SCMOD_ALT | SCMOD_SHIFT))) {
        ft_toggle();
        return;
    }

    /* File tree: when focused, route all keys to it */
    if (ft_focused && ft_handle_key(emit_key, sci_mods)) return;

    /* Oferecer ao Lua primeiro */
    if (emit("key", LUA_TNUMBER, emit_key, LUA_TNUMBER, sci_mods, -1)) return;

    SciObject *key_target = (command_entry_active && command_entry) ?
                            command_entry : focused_view;

    /* No target available - ignore key */
    if (!key_target) return;

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
    /* Function keys (F1-F12) use GDK-style codes >= 0xFF00. Arrows, Home/End,
     * PgUp/PgDn, Delete and Insert use this fork's small SCK_* values (300-315,
     * see Scintilla.h) which fall inside 33..0x10FFFF, so navigating with
     * arrow keys was treated as typing a word character — it opened or
     * extended an undo group instead of leaving it alone, corrupting undo
     * grouping around cursor movement. Exclude both ranges explicitly. */
    bool is_special_key = (emit_key == SCK_UP || emit_key == SCK_DOWN ||
                            emit_key == SCK_LEFT || emit_key == SCK_RIGHT ||
                            emit_key == SCK_HOME || emit_key == SCK_END ||
                            emit_key == SCK_PRIOR || emit_key == SCK_NEXT ||
                            emit_key == SCK_DELETE || emit_key == SCK_INSERT ||
                            emit_key >= 0xFF00);
    bool is_word_char = (emit_key >= 33 && emit_key <= 0x10FFFF) && !is_special_key
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
     * so scinterm does not misread 8→BS, 9→Tab, 13→CR.
     * Use scinterm_key (NotCurses codes) for scintilla_send_key, not emit_key
     * which contains Scintilla codes (SCK_*). */
    if (remapped_ctrl)
        scintilla_send_key(key_target, scinterm_key, (int)(nc_mods | NCKEY_MOD_CTRL));
    else
        scintilla_send_key(key_target, scinterm_key, (int)nc_mods);
}

/* ------------------------------------------------------------------ */
/* Main event loop                                                       */

int main(int argc, char **argv) {
    /* Select UI color theme via LIMOON_THEME env var.
     * Valid values: green (default), blue, bw */
    const char *th = getenv("LIMOON_THEME");
    if (th) {
        if      (strcmp(th, "blue") == 0) T = &THEME_BLUE;
        else if (strcmp(th, "bw")   == 0) T = &THEME_BW;
        else                              T = &THEME_GREEN;
    }

    /* Allow the user to override graphics protocol detection via LIMOON_GRAPHICS.
     * Valid values: kitty, sixel, none, auto (default).
     * Must be called before scintilla_notcurses_init(). */
    const char *gfx = getenv("LIMOON_GRAPHICS");
    if (gfx) {
        ScintermGraphicsProtocol proto = SCINTERM_GRAPHICS_AUTO;
        if      (strcmp(gfx, "kitty") == 0) proto = SCINTERM_GRAPHICS_KITTY;
        else if (strcmp(gfx, "sixel") == 0) proto = SCINTERM_GRAPHICS_SIXEL;
        else if (strcmp(gfx, "none")  == 0) proto = SCINTERM_GRAPHICS_NONE;
        scinterm_set_graphics_protocol(proto);
    }

    if (!scintilla_notcurses_init()) return 1;

    /* Ignore SIGPIPE: subprocesses closing stdin/stdout mid-write would
     * otherwise terminate Li Moon. We handle EPIPE via write() return
     * values in write_process_input and drain_proc_fd. */
    signal(SIGPIPE, SIG_IGN);

    if (!init_limoon(argc, argv)) {
        scintilla_notcurses_shutdown();
        return 1;
    }

    if (nc) {
        notcurses_mice_enable(nc, NCMICE_ALL_EVENTS);
        /* Disable SIGTSTP/SIGINT/SIGQUIT from terminal line discipline (ISIG).
         * This prevents Ctrl+Z from suspending the process. We handle Ctrl+C
         * ourselves (3x = quit) and Ctrl+Z is undo. */
        notcurses_linesigs_disable(nc);
        /* Drain stale terminal input: terminals with XON/XOFF flow control may
         * inject 0x11 (XON/Ctrl+Q) into the input stream during initialization,
         * which would trigger the quit keybinding before the user does anything. */
        struct ncinput _drain;
        while (notcurses_get_nblock(nc, &_drain) != 0) {}
    }

    struct ncinput ni;
    bool running = true;
    while (running && !want_quit) {
        update_ui();

        uint32_t nc_key;
        while ((nc_key = notcurses_get_nblock(nc, &ni)) != 0) {
            if (nc_key == (uint32_t)-1) break;
            needs_render = true;

            if (nc_key == NCKEY_RESIZE) {
                handle_resize();
                continue;
            }

            if (nckey_mouse_p(nc_key)) {
                /* File tree mouse routing */
                if (ft_visible && ni.x < ft_width &&
                    ni.y >= view_top_row() && nc_key != NCKEY_MOTION) {
                    if (nc_key == NCKEY_BUTTON1 && ni.evtype != NCTYPE_RELEASE) {
                        int row_in_tree = ni.y - view_top_row() - 1; /* -1 for title */
                        int clicked = ft_scroll + row_in_tree;
                        ft_focused = true;
                        if (clicked >= 0 && clicked < ft_count) {
                            ft_cursor = clicked;
                            FTEntry *e = &ft_entries[ft_cursor];
                            if (e->is_dir) {
                                if (e->expanded) ft_collapse(ft_cursor);
                                else ft_expand(ft_cursor);
                            } else {
                                ft_open_file(e->path, false);
                            }
                        }
                        needs_render = true;
                    } else if (nc_key == NCKEY_SCROLL_UP) {
                        if (ft_scroll > 0) ft_scroll--;
                        needs_render = true;
                    } else if (nc_key == NCKEY_SCROLL_DOWN) {
                        if (ft_scroll + 1 < ft_count) ft_scroll++;
                        needs_render = true;
                    }
                    continue;
                }

                /* Tab bar click — y==0 is the tab bar row */
                if (ni.y == 0 && tabs_shown && nc_key == NCKEY_BUTTON1) {
                    if (ni.evtype != NCTYPE_RELEASE)
                        tabbar_click(ni.x, false);
                    continue;
                }

                /* Scrollbar click */
                if (scrollbar_enabled && nc_key == NCKEY_BUTTON1 &&
                    ni.evtype != NCTYPE_RELEASE) {
                    NCPane *sp = ncpane_find_at(root_pane, ni.y, ni.x);
                    if (sp && sp->scrollbar_plane && sp->view) {
                        int sb_x = sp->x + sp->cols - 1;
                        if (ni.x == sb_x) {
                            /* Compute thumb position to decide action */
                            intptr_t first  = SS(sp->view, SCI_GETFIRSTVISIBLELINE, 0, 0);
                            intptr_t total  = SS(sp->view, SCI_GETLINECOUNT, 0, 0);
                            int track_h = sp->rows - 2;
                            int vis     = sp->rows;
                            int max_s   = (int)(total - vis);
                            if (max_s < 0) max_s = 0;
                            int th = (total > 0)
                                ? (int)((double)track_h * vis / total) : track_h;
                            if (th < 1) th = 1;
                            if (th > track_h) th = track_h;
                            int tp = (max_s > 0)
                                ? (int)((double)(track_h - th) * first / max_s) : 0;
                            int track_row = ni.y - sp->y - 1; /* relative to track */
                            if (ni.y == sp->y) {
                                SS(sp->view, SCI_LINESCROLL, 0, -1);
                            } else if (ni.y == sp->y + sp->rows - 1) {
                                SS(sp->view, SCI_LINESCROLL, 0, 1);
                            } else if (track_row < tp) {
                                SS(sp->view, SCI_LINESCROLL, 0, -vis);
                            } else if (track_row >= tp + th) {
                                SS(sp->view, SCI_LINESCROLL, 0, vis);
                            }
                            continue;
                        }
                    }
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
                    /* Scroll-wheel "clicks" (4/5) never get a matching
                     * NCTYPE_RELEASE from the terminal, so latching them into
                     * mouse_pressed_button left it stuck at 4/5 after every
                     * scroll — the next plain NCKEY_MOTION (no button held)
                     * was then misread as an active drag, sending spurious
                     * SCM_DRAG events and corrupting the selection. Only real
                     * buttons (1-3) participate in press/drag/release state. */
                    if (button <= 3) mouse_pressed_button = button;
                    scintilla_send_mouse(mouse_view, SCM_PRESS, button,
                        nc_to_sci_mods(ni.modifiers), ni.y, ni.x);
                }
            } else {
                if (ni.evtype != NCTYPE_RELEASE) {
                    /* ESC + printable = Alt+printable.
                     * Terminals that lack kitty keyboard protocol send Alt+X as
                     * the two-byte sequence ESC X.  notcurses_get_nblock returns
                     * NCKEY_ESC for the ESC byte and leaves X in the buffer.
                     * Read the next byte immediately: if it is a printable ASCII
                     * char with no modifiers, re-inject it as Alt+X. */
                    if (nc_key == NCKEY_ESC && ni.modifiers == 0) {
                        struct ncinput ni2 = {0};
                        uint32_t k2 = notcurses_get_nblock(nc, &ni2);
                        if (k2 != 0 && k2 != (uint32_t)-1 &&
                            !nckey_synthesized_p(k2) &&
                            k2 >= 0x20 && k2 < 0x7f &&
                            ni2.modifiers == 0 &&
                            ni2.evtype != NCTYPE_RELEASE) {
                            ni2.modifiers |= NCKEY_MOD_ALT;
                            handle_keypress(&ni2);
                            continue;
                        }
                        /* Not ESC+printable — process ESC normally. */
                    }
                    handle_keypress(&ni);
                }
            }
        }

        struct timespec sleep_ts = { .tv_sec = 0, .tv_nsec = 10 * 1000000 };
        nanosleep(&sleep_ts, NULL);
    }

    close_limoon();
    scintilla_notcurses_shutdown();
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

/**
 * @brief Compute the on-screen (terminal cell) width of a UTF-8 string.
 *
 * strlen() counts bytes, not cells: Portuguese accented characters (ç, ã,
 * õ, á, é, í, ó, ú, â, ê, ô) are 2 bytes but 1 cell, and emoji/CJK are
 * commonly 1 codepoint but 2 cells. Using strlen() for layout math
 * mis-sizes anything outside ASCII.
 *
 * @param s UTF-8 string. NULL treated as empty.
 * @return Terminal cell count, or the byte length as a conservative
 *   fallback if `s` contains invalid UTF-8.
 */
static int utf8_visual_width(const char *s) {
    if (!s || !*s) return 0;
    int width = ncstrwidth(s, NULL, NULL);
    return width >= 0 ? width : (int)strlen(s);
}

/**
 * @brief Truncate a UTF-8 string to fit within `max_cells` visual cells,
 *   never splitting a codepoint.
 *
 * @param src Source string. NULL treated as empty.
 * @param out Destination buffer (>= 1 byte).
 * @param out_size Bytes available in `out`, including the trailing '\0'.
 * @param max_cells Visual cell budget.
 * @return Bytes written, excluding the trailing '\0'.
 */
static size_t utf8_truncate(const char *src, char *out, size_t out_size, int max_cells) {
    if (!src || !out || out_size == 0) return 0;
    size_t written = 0;
    int cells_used = 0;
    const char *p = src;
    while (*p && cells_used < max_cells) {
        int cp_len = 1;
        unsigned char b = (unsigned char)*p;
        if      ((b & 0x80) == 0x00) cp_len = 1;
        else if ((b & 0xE0) == 0xC0) cp_len = 2;
        else if ((b & 0xF0) == 0xE0) cp_len = 3;
        else if ((b & 0xF8) == 0xF0) cp_len = 4;

        char cp[5] = {0};
        for (int i = 0; i < cp_len && p[i]; i++) cp[i] = p[i];
        int w = utf8_visual_width(cp);

        if (cells_used + w > max_cells) break;
        if (written + (size_t)cp_len + 1 > out_size) break;

        memcpy(out + written, p, (size_t)cp_len);
        written += (size_t)cp_len;
        cells_used += w;
        p += cp_len;
    }
    out[written] = '\0';
    return written;
}

/**
 * @brief Draw a UTF-8 string on a plane at (y,x), one grapheme cluster at
 *   a time, up to `max_cells` cells wide.
 *
 * @param plane notcurses plane.
 * @param y,x Start cell.
 * @param s UTF-8 string. NULL or empty draws nothing.
 * @param max_cells Cell budget.
 * @return Cells actually drawn.
 */
static int draw_utf8(struct ncplane *plane, int y, int x, const char *s, int max_cells) {
    if (!s || !*s) return 0;
    const char *p = s;
    int cursor_x = x;
    int cells = 0;
    while (*p && cells < max_cells) {
        size_t consumed = 0;
        int cell_w = ncplane_putegc_yx(plane, y, cursor_x, p, &consumed);
        if (cell_w < 0 || consumed == 0) break; /* invalid byte sequence */
        if (cells + cell_w > max_cells) break; /* wide glyph would overflow the budget */
        cursor_x += cell_w;
        cells += cell_w;
        p += consumed;
    }
    return cells;
}

/**
 * @brief Advance one codepoint forward in a UTF-8 string, without
 *   splitting a multi-byte sequence.
 *
 * @param s UTF-8 string.
 * @param pos Current byte offset into `s`.
 * @return Byte offset of the start of the next codepoint, or `pos`
 *   unchanged if already at the terminating NUL.
 */
static int utf8_next(const char *s, int pos) {
    if (!s[pos]) return pos;
    int n = 1;
    while (s[pos + n] && (s[pos + n] & 0xC0) == 0x80) n++;
    return pos + n;
}

/**
 * @brief Retreat one codepoint backward in a UTF-8 string, without
 *   splitting a multi-byte sequence.
 *
 * @param s UTF-8 string.
 * @param pos Current byte offset into `s`.
 * @return Byte offset of the start of the previous codepoint, or 0 if
 *   `pos` was already at or before the start of the string.
 */
static int utf8_prev(const char *s, int pos) {
    if (pos <= 0) return 0;
    int n = pos - 1;
    while (n > 0 && ((unsigned char)s[n] & 0xC0) == 0x80) n--;
    return n;
}

/* Set gradient color for position t within a tab of total columns */
static void set_tab_color(struct ncplane *plane, int t, int total, bool active) {
    int r, g, b;
    if (active) {
        lerp_rgb(t, total,
                 (T->tab_act_a>>16)&0xFF, (T->tab_act_a>>8)&0xFF, T->tab_act_a&0xFF,
                 (T->tab_act_b>>16)&0xFF, (T->tab_act_b>>8)&0xFF, T->tab_act_b&0xFF,
                 &r, &g, &b);
        ncplane_set_bg_rgb8(plane, r, g, b);
        TH_FG(plane, T->tab_act_text);
    } else {
        lerp_rgb(t, total,
                 (T->tab_ina_a>>16)&0xFF, (T->tab_ina_a>>8)&0xFF, T->tab_ina_a&0xFF,
                 (T->tab_ina_b>>16)&0xFF, (T->tab_ina_b>>8)&0xFF, T->tab_ina_b&0xFF,
                 &r, &g, &b);
        ncplane_set_bg_rgb8(plane, r, g, b);
        TH_FG(plane, T->tab_ina_text);
    }
}

static void draw_tabbar(void) {
    if (!tabbar_plane || !tabs_shown) return;

    unsigned tcols = ncplane_dim_x(tabbar_plane);
    ncplane_erase(tabbar_plane);

    /* Fill remaining area with theme background */
    TH_FG(tabbar_plane, T->tabbar_sep);
    TH_BG(tabbar_plane, T->tabbar_bg);

    int x = 0;
    for (int i = 0; i < num_tabs && x < (int)tcols; i++) {
        bool active = (i == active_tab);
        const char *lbl = tabs[i].label[0] ? tabs[i].label : "Untitled";

        /* Tab layout:  SP label SP [×] SP
         * Minimum width: 1+1+1+3+1 = 7 columns
         * UTF-8 aware: byte count != cell count (see utf8_visual_width). */
        int full_width = utf8_visual_width(lbl);
        int labcols = full_width;
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

        /* Label characters: walk one grapheme cluster at a time (never split
         * a UTF-8 codepoint), budgeted in cells (labcols), not bytes. */
        bool truncated = (labcols < full_width);
        int cell_budget = truncated ? labcols - 1 : labcols; /* reserve 1 cell for … */
        const char *p = lbl;
        int used = 0;
        while (*p && used < cell_budget) {
            size_t consumed = 0;
            set_tab_color(tabbar_plane, x - t0, tabw, active);
            int w = ncplane_putegc_yx(tabbar_plane, 0, x, p, &consumed);
            if (w < 0 || consumed == 0) break; /* invalid byte sequence */
            if (used + w > cell_budget) break; /* wide glyph would overflow the budget */
            x += w;
            used += w;
            p += consumed;
        }
        if (truncated) {
            /* Draw UTF-8 ellipsis … (U+2026, 3 bytes) */
            set_tab_color(tabbar_plane, x - t0, tabw, active);
            ncplane_putstr_yx(tabbar_plane, 0, x, "\xe2\x80\xa6");
            x++;
        }

        /* Space before close button */
        set_tab_color(tabbar_plane, x - t0, tabw, active);
        ncplane_putchar_yx(tabbar_plane, 0, x++, ' ');

        /* [ */
        set_tab_color(tabbar_plane, x - t0, tabw, active);
        ncplane_putchar_yx(tabbar_plane, 0, x++, '[');

        /* × — close button */
        TH_FG(tabbar_plane, active ? T->tab_close_act : T->tab_close_ina);
        /* keep gradient bg */
        { int r, g, b;
          uint32_t ca = active ? T->tab_act_a : T->tab_ina_a;
          uint32_t cb = active ? T->tab_act_b : T->tab_ina_b;
          lerp_rgb(x - t0, tabw,
                   (ca>>16)&0xFF,(ca>>8)&0xFF,ca&0xFF,
                   (cb>>16)&0xFF,(cb>>8)&0xFF,cb&0xFF,
                   &r, &g, &b);
          ncplane_set_bg_rgb8(tabbar_plane, r, g, b); }
        tab_close_x[i] = x;
        ncplane_putstr_yx(tabbar_plane, 0, x++, "\xc3\x97"); /* × U+00D7 */

        /* ] */
        set_tab_color(tabbar_plane, x - t0, tabw, active);
        ncplane_putchar_yx(tabbar_plane, 0, x++, ']');

        /* Trailing separator space */
        TH_FG(tabbar_plane, T->tabbar_sep);
        TH_BG(tabbar_plane, T->tabbar_bg);
        ncplane_putchar_yx(tabbar_plane, 0, x++, ' ');

        tab_x1[i] = x;
    }

    /* Fill rest of bar */
    ncplane_set_fg_default(tabbar_plane);
    TH_BG(tabbar_plane, T->tabbar_bg);
    for (int cx = x; cx < (int)tcols; cx++)
        ncplane_putchar_yx(tabbar_plane, 0, cx, ' ');
    ncplane_set_bg_default(tabbar_plane);
}

/* Draw the status bar onto statusbar_plane (1 row, full width).
 * Left side: statusbar_text0 (plugin space via ui.statusbar_text).
 * Right side: statusbar_text1 (file info via ui.buffer_statusbar_text). */
static void draw_statusbar(void) {
    if (!statusbar_plane || !statusbar_visible) return;

    unsigned scols = ncplane_dim_x(statusbar_plane);
    ncplane_erase(statusbar_plane);

    TH_BG(statusbar_plane, T->sb_bg);
    TH_FG(statusbar_plane, T->sb_fg);

    /* Fill entire row with background first */
    for (int cx = 0; cx < (int)scols; cx++)
        ncplane_putchar_yx(statusbar_plane, 0, cx, ' ');

    /* Left side: plugin/statusbar_text0 */
    const char *left = statusbar_text0;
    int llen = 0;
    if (left && left[0]) {
        int budget = (int)scols - 1;
        int drawn = draw_utf8(statusbar_plane, 0, 1, left, budget > 0 ? budget : 0);
        llen = drawn + 1; /* leading space */
    }

    /* Right side: file info / statusbar_text1 */
    const char *right = statusbar_text1;
    if (right && right[0]) {
        int rlen = utf8_visual_width(right);
        int rx = (int)scols - rlen - 1;
        if (rx > llen + 1) {
            /* Separator between left and right sections */
            if (llen > 0) {
                TH_FG(statusbar_plane, T->sb_sep);
                ncplane_putchar_yx(statusbar_plane, 0, llen + 1, '|');
                TH_FG(statusbar_plane, T->sb_fg);
            }
            draw_utf8(statusbar_plane, 0, rx, right, (int)scols - rx);
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
    current_view = malloc(sizeof(View));
    if (!current_view) return;
    memset(current_view, 0, sizeof(View));

    SciObject *sci = get_view();
    if (!sci) {
        free(current_view);
        current_view = NULL;
        return;
    }

    current_view->sci = sci;
    current_view->plane = scintilla_get_plane(sci);

    if (current_view->plane) {
        nc = ncplane_notcurses(current_view->plane);

        unsigned rows, cols;
        ncplane_dim_yx(notcurses_stdplane(nc), &rows, &cols);
        unsigned view_h = rows > view_overhead() ? rows - view_overhead() : 1;
        ncplane_resize_simple(current_view->plane, view_h, cols);
        ncplane_move_yx(current_view->plane, view_top_row(), 0);
        scintilla_resize(sci);

        /* Initialize root pane tree with the initial view */
        root_pane = ncpane_new(sci);
        if (root_pane) {
            root_pane->y = view_top_row(); root_pane->x = 0;
            root_pane->rows = (int)view_h; root_pane->cols = (int)cols;
        }

        /* Create the dedicated tab bar plane (row 0, full width, 1 row) */
        struct ncplane_options tbopt = {
            .y = 0, .x = 0, .rows = 1, .cols = cols, .name = "tabbar",
        };
        tabbar_plane = ncplane_create(notcurses_stdplane(nc), &tbopt);
        if (tabbar_plane && !tabs_shown)
            ncplane_move_yx(tabbar_plane, -1, 0); /* park off-screen until shown */

        /* Create the status bar plane (last row, full width, 1 row) */
        struct ncplane_options sbopt = {
            .y = (int)rows - 1, .x = 0, .rows = 1, .cols = cols, .name = "statusbar",
        };
        statusbar_plane = ncplane_create(notcurses_stdplane(nc), &sbopt);
    }

    /* Initialize find/replace button and option pointers.
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

    focus_view(sci);
}

void set_title(const char *title) {
    (void)title; /* Terminal title managed by notcurses; no-op */
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
    return scintilla_new(notified, NULL);
}

void focus_view(SciObject *view) {
    if (!view) return;
    if (focused_view && focused_view != view)
        scintilla_set_focus(focused_view, false);
    scintilla_set_focus(view, true);
    focused_view = view;
    /* Update current_view to track the focused sci and its plane */
    if (current_view) {
        current_view->sci = view;
        current_view->plane = scintilla_get_plane(view);
    }
}

sptr_t SS(SciObject *view, int message, uptr_t wparam, sptr_t lparam) {
    return scintilla_send_message(view, message, wparam, lparam);
}

/* ------------------------------------------------------------------ */
/* Split/pane (Bug H — stub: proper pane tree requires significant work) */

void split_view(SciObject *view, SciObject *view2, bool vertical) {
    if (!nc || !root_pane || !view || !view2) return;
    NCPane *pane = ncpane_find_sci(root_pane, view);
    if (!pane || pane->type != NC_SINGLE) return;

    int rows = pane->rows, cols = pane->cols, y = pane->y, x = pane->x;
    NCPane *c1 = ncpane_new(pane->view);
    NCPane *c2 = ncpane_new(view2);
    if (!c1 || !c2) { free(c1); free(c2); return; }

    struct ncplane_options spopt;
    memset(&spopt, 0, sizeof(spopt));
    int split_pos;
    if (vertical) {
        split_pos = cols / 2;
        spopt.rows = (unsigned)rows; spopt.cols = 1;
        spopt.y = y; spopt.x = x + split_pos;
        ncpane_resize(c1, rows, split_pos, y, x);
        ncpane_resize(c2, rows, cols - split_pos - 1, y, x + split_pos + 1);
    } else {
        split_pos = rows / 2;
        spopt.rows = 1; spopt.cols = (unsigned)cols;
        spopt.y = y + split_pos; spopt.x = x;
        ncpane_resize(c1, split_pos, cols, y, x);
        ncpane_resize(c2, rows - split_pos - 1, cols, y + split_pos + 1, x);
    }
    pane->split_plane = ncplane_create(notcurses_stdplane(nc), &spopt);
    if (!pane->split_plane) {
        /* Split plane creation failed — undo the split */
        /* Limpar corretamente os panes filhos */
        pane->view = c1 ? c1->view : NULL;
        if (c1) { c1->view = NULL; ncpane_free(c1, NULL); }
        if (c2) { c2->view = NULL; ncpane_free(c2, NULL); }
        return;
    }
    pane->type      = vertical ? NC_VSPLIT : NC_HSPLIT;
    pane->view      = NULL;
    pane->child1    = c1;
    pane->child2    = c2;
    pane->split_pos = split_pos;
}

bool unsplit_view(SciObject *view, void (*delete_view_fn)(SciObject *)) {
    if (!root_pane || !view) return false;
    NCPane *target = ncpane_find_sci(root_pane, view);
    if (!target) return false;
    NCPane *parent = ncpane_get_parent(root_pane, (void *)view);
    if (!parent) return false; /* already the only pane */

    int py = parent->y, px = parent->x;
    int prows = parent->rows, pcols = parent->cols;
    NCPane *sibling = (parent->child1 == target) ? parent->child2 : parent->child1;

    ncpane_free(sibling, delete_view_fn);

    if (parent->split_plane) {
        ncplane_destroy(parent->split_plane);
        parent->split_plane = NULL;
    }
    parent->type   = NC_SINGLE;
    parent->view   = target->view;
    parent->child1 = NULL;
    parent->child2 = NULL;
    free(target);

    ncpane_resize(parent, prows, pcols, py, px);
    return true;
}

void delete_scintilla(SciObject *view) { scintilla_delete(view); }

Pane *get_top_pane(void) { return (Pane *)root_pane; }

PaneInfo get_pane_info(Pane *pane) {
    NCPane *np = (NCPane *)pane;
    PaneInfo info = {0};
    if (!np) return info;
    info.self   = pane;
    info.width  = np->cols;
    info.height = np->rows;
    if (np->type == NC_SINGLE) {
        info.view = np->view;
    } else {
        info.is_split  = true;
        info.vertical  = (np->type == NC_VSPLIT);
        info.child1    = (Pane *)np->child1;
        info.child2    = (Pane *)np->child2;
        info.split_pos = np->split_pos;
    }
    return info;
}

PaneInfo get_parent_pane_info(PaneInfo info) {
    NCPane *parent = ncpane_get_parent(root_pane, (void *)info.self);
    return get_pane_info((Pane *)parent);
}

PaneInfo get_pane_info_from_view(SciObject *view) {
    return get_pane_info((Pane *)ncpane_find_sci(root_pane, view));
}

void set_pane_split_pos(Pane *pane, int pos) {
    NCPane *np = (NCPane *)pane;
    if (!np || np->type == NC_SINGLE) return;
    np->split_pos = pos;
    ncpane_resize(np, np->rows, np->cols, np->y, np->x);
}

/* ------------------------------------------------------------------ */
/* Tab functions                                                         */

void show_tabs(bool show) {
    if (tabs_shown == show) {
        if (show && tabbar_plane) draw_tabbar();
        return;
    }
    tabs_shown = show;
    if (tabbar_plane) {
        if (show) {
            ncplane_move_yx(tabbar_plane, 0, 0); /* restore from off-screen */
            draw_tabbar();
            ncplane_move_top(tabbar_plane);
        } else {
            ncplane_erase(tabbar_plane);
            ncplane_move_yx(tabbar_plane, -1, 0); /* park off-screen */
        }
    }
    /* Expand/shrink pane tree to reclaim or yield the tabbar row */
    if (nc && root_pane) {
        unsigned rows, cols;
        ncplane_dim_yx(notcurses_stdplane(nc), &rows, &cols);
        unsigned over = view_overhead();
        unsigned view_h = rows > over ? rows - over : 1;
        int ed_x = view_left_col();
        int ed_w = view_edit_cols((int)cols);
        if (ed_w < 1) ed_w = 1;
        ncpane_resize(root_pane, (int)view_h, ed_w, view_top_row(), ed_x);
    }
}

void add_tab(void) {
    if (num_tabs >= MAX_TABS) return;
    tabs[num_tabs].label[0] = '\0';
    tabs[num_tabs].used = true;
    num_tabs++;
    needs_render = true;
}

void set_tab(int index) {
    if (index >= 0) {
        end_word_group(); /* close any open word undo group on buffer switch */
        active_tab = index;
        needs_render = true;
    }
}

void set_tab_label(int index, const char *text) {
    if (index < 0 || index >= num_tabs) return;
    if (text)
        snprintf(tabs[index].label, sizeof(tabs[index].label), "%s", text);
    else
        tabs[index].label[0] = '\0';
    needs_render = true;
}

void move_tab(int from, int to) {
    if (from < 0 || from >= num_tabs || to < 0 || to >= num_tabs || from == to) return;
    TabEntry tmp = tabs[from];
    if (from < to)
        memmove(&tabs[from], &tabs[from + 1], (to - from) * sizeof(TabEntry));
    else
        memmove(&tabs[to + 1], &tabs[to], (from - to) * sizeof(TabEntry));
    tabs[to] = tmp;
    needs_render = true;
}

void remove_tab(int index) {
    if (index < 0 || index >= num_tabs) return;
    memmove(&tabs[index], &tabs[index + 1], (num_tabs - index - 1) * sizeof(TabEntry));
    num_tabs--;
    if (active_tab >= num_tabs && num_tabs > 0) active_tab = num_tabs - 1;
    needs_render = true;
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
static void push_history(char hist[][256], int *n, const char *text) {
    if (!text || !*text) return;
    for (int i = 0; i < *n; i++) {
        if (strcmp(hist[i], text) == 0) {
            if (i == 0) return; /* already at front */
            char tmp[256];
            memcpy(tmp, hist[i], sizeof(tmp));
            memmove(&hist[1], &hist[0], (size_t)i * sizeof(hist[0]));
            memcpy(hist[0], tmp, sizeof(tmp));
            return;
        }
    }
    if (*n < FIND_HIST_MAX) (*n)++;
    memmove(&hist[1], &hist[0], (size_t)(*n - 1) * sizeof(hist[0]));
    snprintf(hist[0], 256, "%s", text);
}
void add_to_find_history(const char *text) { push_history(find_hist, &find_hist_n, text); }
void add_to_repl_history(const char *text) { push_history(repl_hist, &repl_hist_n, text); }
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
    fb_fg(p, focused ? T->fb_cursor_fg : fg);
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
    if (cur < 0) cur = 0;
    if (cur > len) cur = len;

    /* `cur` is a byte offset kept codepoint-aligned by the find-bar's
     * LEFT/RIGHT/Backspace/Delete handlers (see utf8_next()/utf8_prev()).
     * Convert to a cell offset here so the w-cell viewport scrolls
     * correctly for multi-byte, non-1-cell-wide UTF-8 (Portuguese accents,
     * wide glyphs, ...) instead of assuming 1 byte == 1 cell. */
    int cur_cell = 0;
    for (int b = 0; b < cur; ) {
        int nb = utf8_next(text, b);
        int gl = nb - b > 4 ? 4 : nb - b;
        char glyph[5] = {0};
        memcpy(glyph, text + b, (size_t)gl);
        cur_cell += utf8_visual_width(glyph);
        b = nb;
    }
    int off_cell = (cur_cell >= w) ? cur_cell - w + 1 : 0;

    /* Opening bracket */
    fb_fg(p, focused ? c_border : T->fb_dim);
    fb_bg(p, bg_entry);
    ncplane_putchar_yx(p, row, x, focused ? '\xe2' : '['); /* draw later */
    /* Actually use plain bracket always, color indicates focus */
    ncplane_putchar_yx(p, row, x, '[');

    /* Characters: walk codepoint by codepoint (byte-safe), drawing only
     * the glyphs that fall inside the [off_cell, off_cell + w) viewport. */
    int b = 0, cell_col = 0;
    while (b < len && cell_col < off_cell + w) {
        int nb = utf8_next(text, b);
        int gl = nb - b > 4 ? 4 : nb - b;
        char glyph[5] = {0};
        memcpy(glyph, text + b, (size_t)gl);
        int gw = utf8_visual_width(glyph);
        if (gw <= 0) gw = 1;
        if (cell_col >= off_cell) {
            bool is_cur = focused && (cell_col == cur_cell);
            fb_fg(p, is_cur ? T->fb_cursor_fg : fg_text);
            fb_bg(p, is_cur ? c_cursor : bg_entry);
            size_t consumed = 0;
            ncplane_putegc_yx(p, row, x + 1 + (cell_col - off_cell), glyph, &consumed);
        }
        cell_col += gw;
        b = nb;
    }
    /* Blank-fill the rest of the viewport (past end of text, or a wide
     * glyph straddling the right edge). */
    for (int c = cell_col - off_cell; c < w; c++) {
        if (c < 0) continue;
        bool is_cur = focused && (off_cell + c == cur_cell);
        fb_fg(p, is_cur ? T->fb_cursor_fg : fg_text);
        fb_bg(p, is_cur ? c_cursor : bg_entry);
        ncplane_putchar_yx(p, row, x + 1 + c, ' ');
    }

    /* Closing bracket */
    fb_fg(p, focused ? c_border : T->fb_dim);
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

    /* ── Palette from active theme ── */
    const uint32_t C_BAR    = T->fb_bar;
    const uint32_t C_ENTRY  = T->fb_entry;
    const uint32_t C_BORDER = T->fb_border;
    const uint32_t C_LABEL  = T->fb_label;
    const uint32_t C_TEXT   = T->fb_text;
    const uint32_t C_CURSOR = T->fb_cursor;
    const uint32_t C_BTN    = T->fb_btn;
    const uint32_t C_NAV    = T->fb_nav;
    const uint32_t C_HIALL  = T->fb_hiall;
    const uint32_t C_CLOSE  = T->fb_close;
    const uint32_t C_REPL   = T->fb_repl;
    const uint32_t C_OPT_ON  = T->fb_opt_on;
    const uint32_t C_OPT_OFF = T->fb_opt_off;

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

/* Insert a Unicode codepoint (already > 0x7e) into a text buffer at *cur.
 * Returns true on success, false if buffer is too small. */
static bool fb_insert_uni(char *buf, int bufsz, int *cur, uint32_t cp) {
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
    if (len + n >= bufsz) return false;  /* Buffer full */
    memmove(buf + *cur + n, buf + *cur, (size_t)(len - *cur + 1));
    memcpy(buf + *cur, enc, (size_t)n);
    *cur += n;
    return true;
}

void focus_find(void) {
    if (!nc) return;
    if (find_visible) return;
    find_visible = true;

    unsigned rows, cols;
    ncplane_dim_yx(notcurses_stdplane(nc), &rows, &cols);

    /* Shrink main view by 3 rows for findbar */
    if (root_pane) {
        unsigned over = view_overhead();
        unsigned main_h = rows > over + 3 ? rows - over - 3 : 1;
        int ed_x = view_left_col(), ed_w = view_edit_cols((int)cols);
        if (ed_w < 1) ed_w = 1;
        ncpane_resize(root_pane, (int)main_h, ed_w, view_top_row(), ed_x);
    }

    emit("find_pane_show", -1);

    /* Create find bar plane: 3 rows, immediately above the statusbar */
    int fp_y = statusbar_row(rows) - 3;
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

    /* History cycling state (-1 = not cycling; 0 = most recent entry) */
    int find_hist_pos = -1;
    int repl_hist_pos = -1;

    FBHit hits[32];
    int nhits = 0;
    struct ncinput ni;
    bool done = false;

    while (!done) {
        /* Re-render the editor view(s) so find results are visible */
        if (root_pane)
            ncpane_render(root_pane);
        else if (current_view && current_view->sci)
            scintilla_render(current_view->sci);
        draw_findbar(fp, (int)cols, ftext, fcur, rtext, rcur,
                     focus_order[focus_pos], hits, &nhits);
        notcurses_render(nc);
        notcurses_get_blocking(nc, &ni);
        if (ni.evtype == NCTYPE_RELEASE) continue;

        uint32_t k = ni.id;

        /* Handle terminal resize */
        if (k == NCKEY_RESIZE) {
            notcurses_refresh(nc, NULL, NULL);
            unsigned rows2, cols2;
            ncplane_dim_yx(notcurses_stdplane(nc), &rows2, &cols2);
            if (tabbar_plane) ncplane_resize_simple(tabbar_plane, 1, cols2);
            if (statusbar_plane) {
                ncplane_resize_simple(statusbar_plane, 1, cols2);
                ncplane_move_yx(statusbar_plane, (int)rows2 - 1, 0);
            }
            if (root_pane) {
                unsigned over = view_overhead();
                unsigned vh = rows2 > over + 3 ? rows2 - over - 3 : 1;
                int ed_x2 = view_left_col(), ed_w2 = view_edit_cols((int)cols2);
                if (ed_w2 < 1) ed_w2 = 1;
                ncpane_resize(root_pane, (int)vh, ed_w2, view_top_row(), ed_x2);
            }
            fp_y = statusbar_row(rows2) - 3;
            if (fp_y < 1) fp_y = 1;
            ncplane_move_yx(fp, fp_y, 0);
            ncplane_resize_simple(fp, 3, cols2);
            cols = cols2;
            continue;
        }

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
            if (cur_focus == FOCUS_FIND_ENTRY) {
                /* Cycle find history: Up = older entry */
                int next = find_hist_pos + 1;
                if (next < find_hist_n) {
                    find_hist_pos = next;
                    snprintf(ftext, sizeof(ftext), "%s", find_hist[find_hist_pos]);
                    fcur = (int)strlen(ftext);
                }
            } else {
                /* From replace entry or buttons: go to find entry */
                for (int fi = 0; fi < FOCUS_COUNT; fi++)
                    if (focus_order[fi] == FOCUS_FIND_ENTRY) { focus_pos = fi; break; }
            }
        } else if (k == NCKEY_DOWN) {
            if (cur_focus == FOCUS_FIND_ENTRY) {
                if (find_hist_pos > 0) {
                    /* Step forward in history (toward more recent) */
                    find_hist_pos--;
                    snprintf(ftext, sizeof(ftext), "%s", find_hist[find_hist_pos]);
                    fcur = (int)strlen(ftext);
                } else {
                    /* Already at current text: move down to replace entry */
                    find_hist_pos = -1;
                    for (int fi = 0; fi < FOCUS_COUNT; fi++)
                        if (focus_order[fi] == FOCUS_REPL_ENTRY) { focus_pos = fi; break; }
                }
            } else if (cur_focus == FOCUS_REPL_ENTRY) {
                /* Cycle replace history: Down = older entry */
                int next = repl_hist_pos + 1;
                if (next < repl_hist_n) {
                    repl_hist_pos = next;
                    snprintf(rtext, sizeof(rtext), "%s", repl_hist[repl_hist_pos]);
                    rcur = (int)strlen(rtext);
                }
            } else {
                /* From buttons: go to replace entry */
                for (int fi = 0; fi < FOCUS_COUNT; fi++)
                    if (focus_order[fi] == FOCUS_REPL_ENTRY) { focus_pos = fi; break; }
            }

        } else if (k == NCKEY_LEFT) {
            if (on_entry) {
                /* Byte-wise (*cur)-- could land mid-codepoint on multi-byte
                 * UTF-8 (Portuguese accents, etc.), corrupting the byte
                 * offset that fb_draw_entry() and Backspace/Delete rely on
                 * always being codepoint-aligned. */
                char *tx  = (cur_focus == FOCUS_FIND_ENTRY) ? ftext : rtext;
                int  *cur = (cur_focus == FOCUS_FIND_ENTRY) ? &fcur : &rcur;
                *cur = utf8_prev(tx, *cur);
            }
        } else if (k == NCKEY_RIGHT) {
            if (on_entry) {
                char *tx  = (cur_focus == FOCUS_FIND_ENTRY) ? ftext : rtext;
                int  *cur = (cur_focus == FOCUS_FIND_ENTRY) ? &fcur : &rcur;
                *cur = utf8_next(tx, *cur);
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
            /* Any manual edit breaks the history cycle */
            if (cur_focus == FOCUS_FIND_ENTRY) find_hist_pos = -1;
            else repl_hist_pos = -1;
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
    if (nc && root_pane) {
        unsigned rows2, cols2;
        ncplane_dim_yx(notcurses_stdplane(nc), &rows2, &cols2);
        unsigned over = view_overhead();
        unsigned main_h = rows2 > over ? rows2 - over : 1;
        int ed_x3 = view_left_col(), ed_w3 = view_edit_cols((int)cols2);
        if (ed_w3 < 1) ed_w3 = 1;
        ncpane_resize(root_pane, (int)main_h, ed_w3, view_top_row(), ed_x3);
    }
    emit("find_pane_hide", -1);
    find_visible = false;
    notcurses_render(nc);
}

/* ------------------------------------------------------------------ */
/* Command entry (Bug I)                                                 */

static void resize_views_for_command_entry(bool active) {
    if (!nc || !root_pane) return;
    unsigned rows, cols;
    ncplane_dim_yx(notcurses_stdplane(nc), &rows, &cols);
    unsigned ce_h = command_entry_height_stored > 0 ?
                    (unsigned)command_entry_height_stored : 1;
    unsigned over = view_overhead();

    unsigned main_h;
    if (active)
        main_h = rows > over + ce_h ? rows - over - ce_h : 1;
    else
        main_h = rows > over ? rows - over : 1;

    {
        int ed_x4 = view_left_col(), ed_w4 = view_edit_cols((int)cols);
        if (ed_w4 < 1) ed_w4 = 1;
        ncpane_resize(root_pane, (int)main_h, ed_w4, view_top_row(), ed_x4);
    }

    if (active && command_entry) {
        struct ncplane *ce_p = scintilla_get_plane(command_entry);
        if (ce_p) {
            int ce_y = statusbar_row(rows) - (int)ce_h;
            if (ce_y < 1) ce_y = 1;
            ncplane_resize_simple(ce_p, ce_h, cols);
            ncplane_move_yx(ce_p, ce_y, 0);
            scintilla_resize(command_entry);
        }
    } else if (!active && command_entry) {
        struct ncplane *ce_p = scintilla_get_plane(command_entry);
        if (ce_p) ncplane_move_bottom(ce_p);
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

/* Send OSC 1337 SetUserVar to tell WezTerm to update window_background_opacity.
 * pct is 0..100; we convert to 0.0..1.0 and base64-encode the decimal string. */
static void write_osc_bg_alpha(int pct) {
    static const char b64[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    /* pct is a transparency percentage (0=opaque, 100=transparent).
     * Convert to WezTerm opacity: opacity = 1.0 - pct/100. */
    char val[16];
    int len = snprintf(val, sizeof(val), "%.2f", (100 - pct) / 100.0);
    /* Base64-encode val */
    char enc[32];
    int i = 0, o = 0;
    while (i < len) {
        int start = i;
        unsigned b0 = (unsigned char)val[i++];
        unsigned b1 = i < len ? (unsigned char)val[i++] : 0;
        unsigned b2 = i < len ? (unsigned char)val[i++] : 0;
        int nb = i - start; /* bytes consumed: 1, 2, or 3 */
        enc[o++] = b64[b0 >> 2];
        enc[o++] = b64[((b0 & 3) << 4) | (b1 >> 4)];
        enc[o++] = nb >= 2 ? b64[((b1 & 0xf) << 2) | (b2 >> 6)] : '=';
        enc[o++] = nb >= 3 ? b64[b2 & 0x3f] : '=';
    }
    enc[o] = '\0';
    /* Write OSC 1337 SetUserVar sequence.  notcurses opens /dev/tty directly;
     * write through /dev/tty to ensure the PTY master (WezTerm) receives it. */
    FILE *tty = fopen("/dev/tty", "w");
    if (tty) {
        fprintf(tty, "\033]1337;SetUserVar=TA_BG_ALPHA=%s\007", enc);
        fclose(tty);
    }
}

void set_bg_alpha(int pct) {
    scintilla_set_bg_alpha(pct);
    write_osc_bg_alpha(pct);
}

bool is_statusbar_visible(void) { return statusbar_visible; }
void set_statusbar_visible(bool visible) {
    if (statusbar_visible == visible) return;
    statusbar_visible = visible;
    if (statusbar_plane) {
        if (visible)
            ncplane_move_top(statusbar_plane);
        else
            ncplane_move_bottom(statusbar_plane);
    }
    /* Expand/shrink pane tree to reclaim or yield the statusbar row */
    if (nc && root_pane) {
        unsigned rows, cols;
        ncplane_dim_yx(notcurses_stdplane(nc), &rows, &cols);
        unsigned over = view_overhead();
        unsigned view_h = rows > over ? rows - over : 1;
        int ed_x5 = view_left_col(), ed_w5 = view_edit_cols((int)cols);
        if (ed_w5 < 1) ed_w5 = 1;
        ncpane_resize(root_pane, (int)view_h, ed_w5, view_top_row(), ed_x5);
    }
}
void set_scrollbar_visible(bool visible) {
    if (scrollbar_enabled == visible) return;
    scrollbar_enabled = visible;
    handle_resize();
}

const char *get_statusbar_text(int bar) {
    if (bar == 0) return statusbar_text0;
    if (bar == 1) return statusbar_text1;
    return "";
}
void set_statusbar_text(int bar, const char *text) {
    if (bar == 0 && text) snprintf(statusbar_text0, sizeof(statusbar_text0), "%s", text);
    else if (bar == 1 && text) snprintf(statusbar_text1, sizeof(statusbar_text1), "%s", text);
    needs_render = true;
}

/* ------------------------------------------------------------------ */
/* Menus                                                                 */

void *read_menu(lua_State *L, int index)   { (void)L; (void)index; return NULL; }
void popup_menu(void *menu, void *userdata){ (void)menu; (void)userdata; }
void set_menubar(lua_State *L, int index)  { (void)L; (void)index; }

/* ------------------------------------------------------------------ */
/* Clipboard                                                             */

char *get_clipboard_text(int *len) {
    /* Try external clipboard tools first (Wayland, X11) */
    const char *cmds[] = { "wl-paste 2>/dev/null", "xclip -o -sel clipboard 2>/dev/null", "xsel -bo 2>/dev/null", NULL };
    for (int i = 0; cmds[i]; i++) {
        FILE *f = popen(cmds[i], "r");
        if (!f) continue;
        char *buf = NULL;
        size_t cap = 0, total = 0;
        char tmp[4096];
        size_t n;
        while ((n = fread(tmp, 1, sizeof(tmp), f)) > 0) {
            if (total + n + 1 > cap) {
                cap = cap ? cap * 2 : 4096;
                while (cap < total + n + 1) cap *= 2;
                char *nb = realloc(buf, cap);
                if (!nb) { free(buf); pclose(f); buf = NULL; break; }
                buf = nb;
            }
            memcpy(buf + total, tmp, n);
            total += n;
        }
        int rc = pclose(f);
        if (rc == 0 && total > 0 && buf) {
            buf[total] = '\0';
            if (len) *len = (int)total;
            return buf;
        }
        free(buf);
    }
    /* Fall back to Scintilla internal clipboard */
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

    /* Geometry check runs every frame (lightweight): detects resize events that
     * slipped through NCKEY_RESIZE and sets needs_render if correction needed. */
    if (root_pane && !command_entry_active && !find_visible) {
        unsigned rows, cols;
        ncplane_dim_yx(notcurses_stdplane(nc), &rows, &cols);
        unsigned over = view_overhead();
        int expected_h = (int)(rows > over ? rows - over : 1);
        int expected_y = view_top_row();
        int expected_x = view_left_col();
        int expected_w = view_edit_cols((int)cols);
        if (expected_w < 1) expected_w = 1;
        if (root_pane->rows != expected_h || root_pane->y != expected_y ||
            root_pane->x != expected_x || root_pane->cols != expected_w) {
            ncpane_resize(root_pane, expected_h, expected_w, expected_y, expected_x);
            needs_render = true;
        }
    }

    if (!needs_render) return;
    needs_render = false;

    if (root_pane) ncpane_render(root_pane);
    if (ft_visible) ft_draw();

    /* Update cursors: focused view gets active cursor; others get inactive cursor */
    if (root_pane) ncpane_update_cursors(root_pane);

    if (command_entry_active && command_entry) {
        scintilla_render(command_entry);
        scintilla_update_cursor(command_entry);
    } else if (focused_view) {
        scintilla_update_cursor(focused_view);
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
    const uint32_t C_BG     = T->dlg_bg;
    const uint32_t C_BORDER = T->dlg_border;
    const uint32_t C_TITLE  = T->dlg_title;
    const uint32_t C_TEXT   = T->dlg_text;
    const uint32_t C_BTN_FG = T->dlg_btn_fg;
    const uint32_t C_BTN_BG = T->dlg_btn_bg;
    const uint32_t C_FOC_FG = T->dlg_foc_fg;
    const uint32_t C_FOC_BG = T->dlg_foc_bg;
    const uint32_t C_ACCEL  = T->dlg_accel;

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
        ncplane_putstr_yx(dplane, 0, w - 1, "╮");
        ncplane_putstr_yx(dplane, h - 1, 0, "╰");
        ncplane_putstr_yx(dplane, h - 1, w - 1, "╯");
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
            int tx = (w - utf8_visual_width(opts.title)) / 2;
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
        DLG_FGHEX(T->dlg_accel); DLG_BGHEX(C_BG); /* hint text */
        ncplane_putstr_yx(dplane, h - 2, 2, "←/→ Tab · Enter · underlined letter");

        notcurses_render(nc);
        notcurses_get_blocking(nc, &ni);
        if (ni.evtype == NCTYPE_RELEASE) continue;

        uint32_t k = ni.id;
        if (k == NCKEY_RESIZE) {
            handle_resize();
            /* Reposition dialog in the center */
            unsigned dr, dc;
            ncplane_dim_yx(notcurses_stdplane(nc), &dr, &dc);
            ncplane_move_yx(dplane, ((int)dr - h) / 2, ((int)dc - w) / 2);
            continue;
        }
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
    /* Paint background */
    TH_FG(dplane, T->dlg_text); TH_BG(dplane, T->dlg_bg);
    for (int r = 0; r < h; r++)
        for (int c = 0; c < w; c++)
            ncplane_putchar_yx(dplane, r, c, ' ');
    /* Border */
    TH_FG(dplane, T->dlg_border); TH_BG(dplane, T->dlg_bg);
    ncplane_putstr_yx(dplane, 0, 0, "╭");
    ncplane_putstr_yx(dplane, 0, w-1, "╮");
    ncplane_putstr_yx(dplane, h-1, 0, "╰");
    ncplane_putstr_yx(dplane, h-1, w-1, "╯");
    for (int c = 1; c < w-1; c++) { ncplane_putstr_yx(dplane,0,c,"─"); ncplane_putstr_yx(dplane,h-1,c,"─"); }
    for (int r = 1; r < h-1; r++) { ncplane_putstr_yx(dplane,r,0,"│"); ncplane_putstr_yx(dplane,r,w-1,"│"); }

    if (opts.title) {
        int tx = (w - utf8_visual_width(opts.title)) / 2;
        if (tx < 0) tx = 0;
        TH_FG(dplane, T->dlg_title); ncplane_set_styles(dplane, NCSTYLE_BOLD);
        ncplane_putstr_yx(dplane, 1, tx, opts.title);
        ncplane_set_styles(dplane, NCSTYLE_NONE);
    }
    TH_FG(dplane, T->dlg_text);
    if (opts.text) ncplane_putstr_yx(dplane, 3, 2, opts.text);

    int iy = 5, ix = 2, iw = w - 4;
    TH_FG(dplane, T->dlg_border);
    ncplane_putstr_yx(dplane, iy, ix, "┌");
    for (int i = 0; i < iw - 2; i++) ncplane_putstr_yx(dplane, iy, ix + 1 + i, "─");
    ncplane_putstr_yx(dplane, iy, ix + iw - 1, "┐");
    ncplane_putstr_yx(dplane, iy + 1, ix, "│");
    ncplane_putstr_yx(dplane, iy + 1, ix + iw - 1, "│");
    ncplane_putstr_yx(dplane, iy + 2, ix, "└");
    for (int i = 0; i < iw - 2; i++) ncplane_putstr_yx(dplane, iy + 2, ix + 1 + i, "─");
    ncplane_putstr_yx(dplane, iy + 2, ix + iw - 1, "┘");
    TH_FG(dplane, T->dlg_text);

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
        if (copy_len > 255) copy_len = 255;
        char visible[256];
        memcpy(visible, buf + offset, copy_len);
        visible[copy_len] = '\0';
        ncplane_printf_yx(dplane, text_y, text_x, "%-*s", max_len, visible);
        ncplane_cursor_move_yx(dplane, text_y, text_x + curpos - offset);
        notcurses_render(nc);

        notcurses_get_blocking(nc, &ni);
        if (ni.evtype == NCTYPE_RELEASE) continue;
        if (ni.id == NCKEY_RESIZE) {
            handle_resize();
            unsigned dr, dc;
            ncplane_dim_yx(notcurses_stdplane(nc), &dr, &dc);
            ncplane_move_yx(dplane, ((int)dr - h) / 2, ((int)dc - w) / 2);
            continue;
        }

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
    size_t path_len = strlen(path);
    while ((ent = readdir(d)) != NULL && n < max) {
        if (strcmp(ent->d_name, ".") == 0) continue;
        /* Allocate dynamically to avoid large stack buffer (PATH_MAX can be large) */
        size_t full_size = path_len + strlen(ent->d_name) + 2; /* / + null */
        char *full = malloc(full_size);
        if (!full) continue;
        snprintf(full, full_size, "%s/%s", path, ent->d_name);
        struct stat st;
        bool is_dir = (stat(full, &st) == 0 && S_ISDIR(st.st_mode));
        free(full);
        if (only_dirs && !is_dir && strcmp(ent->d_name, "..") != 0) continue;
        snprintf(entries[n].name, sizeof(entries[n].name), "%s", ent->d_name);
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
    if (opts.dir && opts.dir[0]) {
        snprintf(cwd, sizeof(cwd), "%s", opts.dir);
    } else if (!getcwd(cwd, sizeof(cwd))) {
        snprintf(cwd, sizeof(cwd), ".");
    }
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
        TH_BG(dp, T->fd_bg); TH_FG(dp, T->fd_idle_fg);
        for (int row = 0; row < dh; row++)
            for (int col = 0; col < dw; col++)
                ncplane_putchar_yx(dp, row, col, ' ');

        /* ── Title (gradient bar) ── */
        draw_gradient_row(dp, 0, dw,
            (T->fd_title_a>>16)&0xFF,(T->fd_title_a>>8)&0xFF, T->fd_title_a&0xFF,
            (T->fd_title_b>>16)&0xFF,(T->fd_title_b>>8)&0xFF, T->fd_title_b&0xFF,
            (T->fd_title_fg>>16)&0xFF,(T->fd_title_fg>>8)&0xFF, T->fd_title_fg&0xFF);
        const char *title = opts.title ? opts.title : (save_mode ? "Save File" : "Open File");
        int tx = (dw - utf8_visual_width(title)) / 2;
        TH_FG(dp, T->fd_title_fg); TH_BG(dp, T->fd_title_bg);
        ncplane_putstr_yx(dp, 0, tx > 0 ? tx : 0, title);

        /* ── Current path ── */
        TH_FG(dp, T->fd_path); TH_BG(dp, T->fd_bg);
        int path_avail = dw - 8;
        const char *pdisp = cwd;
        if ((int)strlen(cwd) > path_avail) pdisp = cwd + strlen(cwd) - path_avail;
        ncplane_printf_yx(dp, path_row, 1, "Path: %.*s", dw - 8, pdisp);

        /* ── Filter entry ── */
        bool filter_focused = (focus == 1);
        TH_BG(dp, filter_focused ? T->fd_entry_focus : T->fd_entry_blur);
        TH_FG(dp, T->fd_entry_text);
        int f_off = filter_cur > entry_w ? filter_cur - entry_w : 0;
        ncplane_printf_yx(dp, filt_row, 1, "Filter:[%-*.*s]",
                          entry_w, entry_w, filter + f_off);

        /* ── Separator ── */
        TH_FG(dp, T->fd_sep); TH_BG(dp, T->fd_bg);
        for (int c = 0; c < dw; c++) ncplane_putchar_yx(dp, sep1_row, c, '-');

        /* ── File list ── */
        if (cur_item < scroll) scroll = cur_item;
        if (cur_item >= scroll + list_h) scroll = cur_item - list_h + 1;

        for (int i = 0; i < list_h; i++) {
            int fi = scroll + i;
            int row = list_top + i;
            if (fi >= n_filtered) {
                /* empty row */
                TH_BG(dp, T->fd_bg);
                for (int c = 0; c < dw; c++) ncplane_putchar_yx(dp, row, c, ' ');
                continue;
            }
            int ei = filtered[fi];
            bool is_dir = entries[ei].is_dir;
            bool sel = (fi == cur_item && focus == 0);

            if (sel) {
                draw_gradient_row(dp, row, dw,
                    (T->fd_sel_a>>16)&0xFF,(T->fd_sel_a>>8)&0xFF, T->fd_sel_a&0xFF,
                    (T->fd_sel_b>>16)&0xFF,(T->fd_sel_b>>8)&0xFF, T->fd_sel_b&0xFF,
                    (T->fd_title_fg>>16)&0xFF,(T->fd_title_fg>>8)&0xFF, T->fd_title_fg&0xFF);
                TH_BG(dp, T->fd_sel_bg);
            } else {
                TH_BG(dp, T->fd_bg);
            }

            /* Icon */
            if (is_dir)
                TH_FG(dp, sel ? T->fd_dir_sel : T->fd_dir_unsel);
            else
                TH_FG(dp, sel ? T->fd_file_sel : T->fd_file_unsel);

            const char *icon = is_dir ? " [/] " : "     ";
            ncplane_putstr_yx(dp, row, 0, icon);

            /* Name, truncated */
            int maxname = dw - 6;
            ncplane_printf_yx(dp, row, 5, "%-*.*s", maxname, maxname, entries[ei].name);
        }

        /* ── Separator bottom ── */
        TH_FG(dp, T->fd_sep); TH_BG(dp, T->fd_bg);
        for (int c = 0; c < dw; c++) ncplane_putchar_yx(dp, sep2_row, c, '-');

        /* ── Filename entry (save mode) ── */
        if (save_mode && fn_row >= 0) {
            bool fn_focused = (focus == 2);
            TH_BG(dp, fn_focused ? T->fd_entry_focus : T->fd_entry_blur);
            TH_FG(dp, T->fd_entry_text);
            int fn_off = fname_cur > entry_w ? fname_cur - entry_w : 0;
            ncplane_printf_yx(dp, fn_row, 1, "Name:  [%-*.*s]",
                              entry_w, entry_w, fname + fn_off);
        }

        /* ── Buttons ── */
        TH_BG(dp, T->fd_bg);
        if (focus == 3) draw_gradient_row(dp, btn_row, dw/2 - 1,
            (T->fd_sel_a>>16)&0xFF,(T->fd_sel_a>>8)&0xFF, T->fd_sel_a&0xFF,
            (T->fd_sel_b>>16)&0xFF,(T->fd_sel_b>>8)&0xFF, T->fd_sel_b&0xFF,
            (T->fd_title_fg>>16)&0xFF,(T->fd_title_fg>>8)&0xFF, T->fd_title_fg&0xFF);
        if (focus == 4) draw_gradient_row(dp, btn_row, dw - dw/2,
            (T->fd_sel_a>>16)&0xFF,(T->fd_sel_a>>8)&0xFF, T->fd_sel_a&0xFF,
            (T->fd_sel_b>>16)&0xFF,(T->fd_sel_b>>8)&0xFF, T->fd_sel_b&0xFF,
            (T->fd_title_fg>>16)&0xFF,(T->fd_title_fg>>8)&0xFF, T->fd_title_fg&0xFF);

        TH_BG(dp, focus == 3 ? T->fd_ok_foc_bg : T->fd_idle_bg);
        TH_FG(dp, focus == 3 ? T->fd_ok_foc_fg : T->fd_idle_fg);
        ncplane_printf_yx(dp, btn_row, dw/2 - 5,
                          focus == 3 ? "[ OK ]" : "  OK  ");

        TH_BG(dp, focus == 4 ? T->fd_cancel_foc_bg : T->fd_idle_bg);
        TH_FG(dp, focus == 4 ? T->fd_cancel_foc_fg : T->fd_idle_fg);
        ncplane_printf_yx(dp, btn_row, dw/2 + 2,
                          focus == 4 ? "[Cancel]" : " Cancel ");

        notcurses_render(nc);
        notcurses_get_blocking(nc, &ni);
        if (ni.evtype == NCTYPE_RELEASE) continue;

        uint32_t k = ni.id;
        if (k == NCKEY_RESIZE) {
            handle_resize();
            unsigned ndr, ndc;
            ncplane_dim_yx(notcurses_stdplane(nc), &ndr, &ndc);
            dw = (int)ndc - 4; if (dw > 88) dw = 88; if (dw < 24) dw = 24;
            dh = (int)ndr - 2; if (dh < 14) dh = 14;
            dy = ((int)ndr - dh) / 2; dx = ((int)ndc - dw) / 2;
            ncplane_resize_simple(dp, (unsigned)dh, (unsigned)dw);
            ncplane_move_yx(dp, dy, dx);
            list_h = (save_mode ? dh - 5 : dh - 3) - list_top;
            if (list_h < 1) list_h = 1;
            continue;
        }

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
                    /* Verificar bounds antes de concatenar */
                    size_t cwd_len = strlen(cwd);
                    size_t fname_len = strlen(fname);
                    if (cwd_len + 1 + fname_len >= sizeof(result)) {
                        /* Path muito longo */
                        accepted = false; done = true;
                    } else {
                        snprintf(result, sizeof(result), "%s/%s", cwd, fname);
                        accepted = true; done = true;
                    }
                } else if (!save_mode && n_filtered > 0) {
                    int ei = filtered[cur_item];
                    if (!entries[ei].is_dir || opts.only_dirs) {
                        size_t cwd_len = strlen(cwd);
                        size_t name_len = strlen(entries[ei].name);
                        if (cwd_len + 1 + name_len >= sizeof(result)) {
                            accepted = false; done = true;
                        } else {
                            snprintf(result, sizeof(result), "%s/%s", cwd, entries[ei].name);
                            accepted = true; done = true;
                        }
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
                            size_t name_len = strlen(entries[ei].name);
                            /* Check bounds: need space for '/' + name + '\0' */
                            if (cl + 1 + name_len < sizeof(cwd)) {
                                snprintf(cwd + cl, sizeof(cwd) - cl, "/%s", entries[ei].name);
                            } else {
                                /* Path muito longo - nao navegar */
                                continue;
                            }
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
        int tx = (w - utf8_visual_width(opts.title)) / 2;
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
    TH_FG(dp, T->dlg_text); TH_BG(dp, T->dlg_bg);
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
    if (!col_w) { ncplane_destroy(dp); return 0; }
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
    if (!row_strs) { free(col_w); ncplane_destroy(dp); return 0; }
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
    if (!filtered) {
        for (int r = 0; r < num_rows; r++) free(row_strs[r]);
        free(row_strs);
        free(col_w);
        ncplane_destroy(dp);
        return 0;
    }
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

        /* Background */
        TH_FG(dp, T->dlg_text); TH_BG(dp, T->dlg_bg);
        for (int r = 0; r < dh; r++)
            for (int c = 0; c < dw; c++) ncplane_putchar_yx(dp, r, c, ' ');
        /* Border */
        TH_FG(dp, T->dlg_border);
        ncplane_putstr_yx(dp,0,0,"╭"); ncplane_putstr_yx(dp,0,dw-1,"╮");
        ncplane_putstr_yx(dp,dh-1,0,"╰"); ncplane_putstr_yx(dp,dh-1,dw-1,"╯");
        for (int c=1;c<dw-1;c++){ncplane_putstr_yx(dp,0,c,"─");ncplane_putstr_yx(dp,dh-1,c,"─");}
        for (int r=1;r<dh-1;r++){ncplane_putstr_yx(dp,r,0,"│");ncplane_putstr_yx(dp,r,dw-1,"│");}

        /* Title */
        if (opts.title) {
            int tx = (dw - utf8_visual_width(opts.title)) / 2;
            TH_FG(dp, T->dlg_title); ncplane_set_styles(dp, NCSTYLE_BOLD);
            ncplane_putstr_yx(dp, 0, tx > 0 ? tx : 0, opts.title);
            ncplane_set_styles(dp, NCSTYLE_NONE);
        }

        /* Filter entry */
        TH_FG(dp, T->dlg_text); TH_BG(dp, T->dlg_bg);
        ncplane_printf_yx(dp, 1, 1, "Filter: [%-*s]", dw - 12, filter);

        /* Column headers */
        if (opts.columns) {
            TH_FG(dp, T->dlg_accel);
            int hx = 1;
            for (int c = 0; c < num_cols; c++) {
                lua_rawgeti(L, opts.columns, c + 1);
                const char *hdr = lua_tostring(L, -1);
                ncplane_printf_yx(dp, 2, hx, "%-*s ", col_w[c],
                                  hdr ? hdr : "");
                lua_pop(L, 1);
                hx += col_w[c] + 1;
            }
            TH_FG(dp, T->dlg_text);
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
                if (sel) { TH_FG(dp, T->dlg_foc_fg); TH_BG(dp, T->dlg_foc_bg); }
                else     { TH_FG(dp, T->dlg_text);   TH_BG(dp, T->dlg_bg); }
                ncplane_printf_yx(dp, list_top + i, 1, sel ? ">%-*s" : " %-*s",
                                  dw - 3, row_strs[ri]);
            }
            TH_FG(dp, T->dlg_text); TH_BG(dp, T->dlg_bg);
        }

        /* Buttons */
        int btn_y = dh - 2;
        int b0x = dw / 2 - (int)strlen(b0) - 3;
        int b1x = dw / 2 + 1;
        TH_FG(dp, cur_btn == 0 ? T->dlg_foc_fg : T->dlg_btn_fg);
        TH_BG(dp, cur_btn == 0 ? T->dlg_foc_bg : T->dlg_btn_bg);
        ncplane_printf_yx(dp, btn_y, b0x, cur_btn == 0 ? "[%s]" : " %s ", b0);
        TH_FG(dp, cur_btn == 1 ? T->dlg_foc_fg : T->dlg_btn_fg);
        TH_BG(dp, cur_btn == 1 ? T->dlg_foc_bg : T->dlg_btn_bg);
        ncplane_printf_yx(dp, btn_y, b1x, cur_btn == 1 ? "[%s]" : " %s ", b1);
        TH_FG(dp, T->dlg_text); TH_BG(dp, T->dlg_bg);

        notcurses_render(nc);
        notcurses_get_blocking(nc, &ni);
        if (ni.evtype == NCTYPE_RELEASE) continue;

        uint32_t k = ni.id;
        if (k == NCKEY_RESIZE) {
            handle_resize();
            unsigned ndr, ndc;
            ncplane_dim_yx(notcurses_stdplane(nc), &ndr, &ndc);
            dw = (int)ndc - 4; if (dw > 80) dw = 80;
            dh = (int)ndr - 4; if (dh < 8) dh = 8;
            dy = ((int)ndr - dh) / 2; dx = ((int)ndc - dw) / 2;
            ncplane_resize_simple(dp, (unsigned)dh, (unsigned)dw);
            ncplane_move_yx(dp, dy, dx);
            list_bot = dh - 3; list_h = list_bot - list_top;
            if (list_h < 1) list_h = 1;
            continue;
        }
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

/* Forward declaration - defined after parse_cmd */
static void free_argv(char **argv, int argc);

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
            char **new_argv = realloc(argv, cap * sizeof(char *));
            if (!new_argv) {
                free_argv(argv, argc);
                *argv_out = NULL;
                return 0;
            }
            argv = new_argv;
        }
        argv[argc] = strdup(tok);
        if (!argv[argc]) {
            free_argv(argv, argc);
            *argv_out = NULL;
            return 0;
        }
        argc++;
    }
    char **new_argv = realloc(argv, (argc + 1) * sizeof(char *));
    if (!new_argv) {
        free_argv(argv, argc);
        *argv_out = NULL;
        return 0;
    }
    argv = new_argv;
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
        if (!envp) {
            free_argv(argv, argc);
            if (error) *error = "out of memory";
            return false;
        }
        for (int i = 0; i < envc; i++) {
            lua_rawgeti(L, envi, i + 1);
            const char *s = lua_tostring(L, -1);
            envp[i] = strdup(s ? s : "");
            lua_pop(L, 1);
            if (!envp[i]) {
                for (int j = 0; j < i; j++) free(envp[j]);
                free(envp);
                free_argv(argv, argc);
                if (error) *error = "out of memory";
                return false;
            }
        }
        envp[envc] = NULL;
    }

    /* Create pipes: [0]=read end, [1]=write end */
    int p_stdin[2] = {-1, -1}, p_stdout[2] = {-1, -1}, p_stderr[2] = {-1, -1};
    if (pipe(p_stdin) < 0 || pipe(p_stdout) < 0 || pipe(p_stderr) < 0) {
        if (error) *error = strerror(errno);
        if (p_stdin[0] >= 0)  { close(p_stdin[0]);  close(p_stdin[1]); }
        if (p_stdout[0] >= 0) { close(p_stdout[0]); close(p_stdout[1]); }
        if (p_stderr[0] >= 0) { close(p_stderr[0]); close(p_stderr[1]); }
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

/* Poll fd with short timeout; render UI on timeout. Returns poll result. */
static int poll_with_ui(int fd) {
    struct pollfd pfd = { .fd = fd, .events = POLLIN };
    int r = poll(&pfd, 1, 50);
    if (r == 0 && nc && root_pane) {
        ncpane_render(root_pane);
        notcurses_render(nc);
    }
    return r;
}

/**
 * @brief Read output from a subprocess's stdout according to `option`.
 *
 * Options:
 *  - 'a': read everything currently available (until EOF).
 *  - 'l': read one line, stripping the trailing newline.
 *  - 'L': read one line, keeping the trailing newline.
 *  - 'n': read exactly `*len` bytes (see `len` below).
 *
 * Buffer growth ('l'/'L'/'a' modes): the capacity guard grows whenever
 * `total >= cap - 1`, so a write always lands at index <= cap - 2 and the
 * loop-terminating `result[total] = '\0'` always lands at index <= cap - 1
 * — both in bounds for a `cap`-sized allocation, including the 'L' case
 * where the loop can write the newline and break in the same iteration.
 *
 * @param len IN for option 'n' (bytes requested); OUT for every option
 *   (bytes actually read, i.e. the returned string's length). Must be
 *   read into a local BEFORE being reset for the output role, or 'n'
 *   always requests zero bytes.
 */
char *read_process_output(Process *proc, char option, size_t *len, const char **error, int *code) {
    NProcess *np = (NProcess *)proc;
    if (!np || np->stdout_fd < 0) {
        if (len) *len = 0;
        if (error) *error = NULL; /* EOF */
        return NULL;
    }

    int fd = np->stdout_fd;
    char *result = NULL;
    /* For option 'n', *len is an INPUT (bytes requested) that must be read
     * before being overwritten below with the OUTPUT (bytes actually read).
     * Reading it after the reset made `want` always 0, so 'n' never read
     * anything. */
    size_t requested_n = (option == 'n') ? *len : 0;
    *len = 0;
    /* Default to "no error" (clean EOF) unless a branch below overwrites this
     * with strerror(). Without this, proc_read()'s `error` local is
     * uninitialized stack garbage whenever a read legitimately returns
     * nothing (e.g. 'a' mode against an empty pipe) — proc_read() then
     * treats that garbage pointer as a real error string and passes it to
     * lua_pushstring(), segfaulting inside Lua's string interner. */
    if (error) *error = NULL;

    if (option == 'n') {
        size_t want = requested_n;
        result = malloc(want + 1);
        if (!result) return NULL;
        ssize_t total = 0;
        while ((size_t)total < want) {
            int r = poll_with_ui(fd);
            if (r < 0) {
                if (error) *error = strerror(errno);
                if (code)  *code  = errno;
                free(result); return NULL;
            }
            if (r == 0) continue; /* timeout — UI was pumped */
            ssize_t n = read(fd, result + total, want - (size_t)total);
            if (n > 0) total += n;
            else break; /* EOF or error */
        }
        result[total] = '\0';
        *len = (size_t)total;
        if (total == 0) { free(result); return NULL; }
    } else if (option == 'a') {
        size_t cap = 4096;
        result = malloc(cap);
        if (!result) return NULL;
        ssize_t total = 0;
        while (true) {
            int r = poll_with_ui(fd);
            if (r < 0) break;
            if (r == 0) continue;
            ssize_t n = read(fd, result + total, cap - (size_t)total - 1);
            if (n <= 0) break;
            total += n;
            if ((size_t)total >= cap - 1) {
                cap *= 2;
                char *nb = realloc(result, cap);
                if (!nb) { free(result); return NULL; }
                result = nb;
            }
        }
        result[total] = '\0';
        *len = (size_t)total;
        if (total == 0) { free(result); return NULL; }
    } else {
        /* 'l' or 'L': read one line */
        size_t cap = 256;
        result = malloc(cap);
        if (!result) return NULL;
        ssize_t total = 0;
        while (true) {
            int r = poll_with_ui(fd);
            if (r < 0) break;
            if (r == 0) continue;
            char ch;
            ssize_t n = read(fd, &ch, 1);
            if (n <= 0) break;
            if ((size_t)total >= cap - 1) {
                cap *= 2;
                char *nb = realloc(result, cap);
                if (!nb) { free(result); return NULL; }
                result = nb;
            }
            if (ch == '\n') {
                if (option == 'L') result[total++] = ch;
                break;
            }
            if (ch != '\r') result[total++] = ch;
        }
        result[total] = '\0';
        *len = (size_t)total;
        if (total == 0) { free(result); return NULL; }
    }
    return result;
}

void write_process_input(Process *proc, const char *s, size_t len) {
    NProcess *np = (NProcess *)proc;
    if (!np || np->stdin_fd < 0) return;
    ssize_t written = 0;
    while ((size_t)written < len) {
        ssize_t n = write(np->stdin_fd, s + written, len - written);
        if (n < 0) {
            if (errno == EINTR) continue;
            break; /* EPIPE, EAGAIN, or real error */
        }
        if (n == 0) break;
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

void suspend(void) {
    if (!nc) return;
    /* Re-enable ISIG so the shell's job control handles the stop correctly */
    notcurses_linesigs_enable(nc);
    /* Suspend the process; execution resumes here after SIGCONT */
    raise(SIGTSTP);
    /* Restore notcurses input handling and repaint */
    notcurses_linesigs_disable(nc);
    notcurses_refresh(nc, NULL, NULL);
}
void quit(void)    { want_quit = true; }
