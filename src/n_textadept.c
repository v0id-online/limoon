/*
 * n_textadept.c - Notcurses frontend for Textadept.
 *
 * This file implements the platform‑dependent functions required by Textadept
 * using the Notcurses library. It replaces the former curses (CDK) frontend.
 *
 * Copyright (c) 2026
 */

#include <notcurses/notcurses.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>
#include "textadept.h"
#include "textadept_platform.h"

/* External Scintilla functions */
extern SciObject *scintilla_new(void (*)(SciObject*,int,SCNotification*,void*), void*);
extern sptr_t scintilla_send_message(SciObject*, int, uptr_t, sptr_t);
extern void scintilla_delete(SciObject*);

/* ------------------------------------------------------------------------ */

int main(int argc, char **argv) {
	/* Initialize Notcurses */
	if (!ensure_notcurses()) {
		fprintf(stderr, "Failed to initialize Notcurses\n");
		return 1;
	}

	/* Call init_textadept which will call new_window etc. */
	if (!init_textadept(argc, argv)) {
		fprintf(stderr, "Failed to initialize Textadept\n");
		notcurses_stop(nc);
		return 1;
	}

	struct ncinput ni;
	bool running = true;
	while (running) {
		/* Process any pending UI updates */
		update_ui();
		/* Non-blocking input */
		while (notcurses_get_nblock(nc, &ni) != -1) {
			if (ni.evtype == NCTYPE_PRESS) {
				/* For now, exit on 'q' or Ctrl+C */
				if (ni.id == 'q' || (ni.id == 'c' && (ni.modifiers & NCTRL_MSK))) {
					running = false;
					break;
				}
				/* TODO: forward keypress to focused view */
			}
		}
		/* Small sleep to reduce CPU usage */
		struct timespec ts = { .tv_sec = 0, .tv_nsec = 10 * 1000000 }; // 10ms
		nanosleep(&ts, NULL);
	}

	/* Cleanup */
	close_textadept();
	notcurses_stop(nc);
	return exit_status;
}
/* Notcurses view management                                                */

typedef struct {
	struct ncplane *plane;
	SciObject *sci;   /* associated Scintilla view (placeholder) */
} View;

static struct notcurses *nc = NULL;
static View *current_view = NULL;
static char find_text[256] = "";
static char repl_text[256] = "";
static bool command_entry_active = false;
static bool statusbar_visible = false;
static char statusbar_text0[256] = "";
static char statusbar_text1[256] = "";

static bool ensure_notcurses(void) {
	if (!nc) {
		nc = notcurses_init(NULL, stdout);
		if (!nc) return false;
	}
	return true;
}

/* Render a string at (y, x) inside the view's plane and refresh the screen */
static void update_view(View *view, int y, int x, const char *str) {
	if (view && view->plane && str) {
		ncplane_putstr_yx(view->plane, y, x, "%s", str);
		notcurses_render(nc);
	}
}

/* Create a new View occupying the whole standard plane */
static View* create_view(void) {
	struct ncplane *std = notcurses_stdplane(nc);
	/* create a subplane that covers the entire standard plane */
	struct ncplane_options opts = {
		.y = 0,
		.x = 0,
		.rows = ncplane_dim_y(std),
		.cols = ncplane_dim_x(std),
		.userptr = NULL,
		.name = "textadept_view",
		.resizecb = NULL,
		.flags = 0,
	};
	struct ncplane *plane = ncplane_create(std, &opts);
	if (!plane) return NULL;

	View *view = malloc(sizeof(View));
	if (!view) {
		ncplane_destroy(plane);
		return NULL;
	}
	memset(view, 0, sizeof(View));
	view->plane = plane;
	view->sci = NULL;   /* will be filled later by new_scintilla() */
	return view;
}

static void destroy_view(View *view) {
	if (!view) return;
	if (view->plane) ncplane_destroy(view->plane);
	free(view);
}

/* ------------------------------------------------------------------------ */
/* Platform functions                                                       */

const char *get_platform(void) {
	return "NOTCURSES";
}

const char *get_charset(void) {
	return "UTF-8";
}

void new_window(SciObject *(*get_view)(void)) {
	if (!ensure_notcurses()) return;
	current_view = create_view();
	if (!current_view) return;
	SciObject *sci = get_view();
	if (sci) {
		current_view->sci = sci;
	}
}

void set_title(const char *title) {
	if (title) {
		printf("\x1b]0;%s\x07", title);
		fflush(stdout);
	}
}

bool is_maximized(void) { return false; }
void set_maximized(bool maximize) { (void)maximize; }
void get_size(int *w, int *h) {
	if (!ensure_notcurses()) { if(w) *w=80; if(h) *h=24; return; }
	struct ncplane *std = notcurses_stdplane(nc);
	if (w) *w = ncplane_dim_x(std);
	if (h) *h = ncplane_dim_y(std);
}
void set_size(int width, int height) { (void)width; (void)height; }

SciObject *new_scintilla(void (*notified)(SciObject *, int, SCNotification *, void *)) {
	return scintilla_new(notified, NULL);
}

void focus_view(SciObject *view) {
	if (focused_view) SS(focused_view, SCI_SETFOCUS, 0, 0);
	SS(view, SCI_SETFOCUS, 1, 0);
	focused_view = view;
}

sptr_t SS(SciObject *view, int message, uptr_t wparam, sptr_t lparam) {
	return scintilla_send_message(view, message, wparam, lparam);
}

void split_view(SciObject *view, SciObject *view2, bool vertical) {
	(void)view; (void)view2;
	if (!ensure_notcurses()) return;
	struct ncplane *std = notcurses_stdplane(nc);
	ncplane_putstr_yx(std, 0, 0, "[Split %s]", vertical ? "vertical" : "horizontal");
	// TODO: actual split pane management with Notcurses planes
}

bool unsplit_view(SciObject *view, void (*delete_view)(SciObject *)) {
	(void)view; (void)delete_view;
	if (!ensure_notcurses()) return false;
	struct ncplane *std = notcurses_stdplane(nc);
	ncplane_putstr_yx(std, 1, 0, "[Unsplit]");
	return false; // no split pane found for the given view (TODO)
}

void delete_scintilla(SciObject *view) {
	scintilla_delete(view);
}

/* Pane functions */
Pane *get_top_pane(void) { return NULL; }
PaneInfo get_pane_info(Pane *pane) { (void)pane; PaneInfo info = {0}; return info; }
PaneInfo get_parent_pane_info(PaneInfo info) { (void)info; PaneInfo ret = {0}; return ret; }
PaneInfo get_pane_info_from_view(SciObject *view) { (void)view; PaneInfo info = {0}; return info; }
void set_pane_split_pos(Pane *pane, int pos) { (void)pane; (void)pos; }

/* Tab functions */
void show_tabs(bool show) { (void)show; }
void add_tab(void) {}
void set_tab(int index) { (void)index; }
void set_tab_label(int index, const char *text) { (void)index; (void)text; }
void move_tab(int from, int to) { (void)from; (void)to; }
void remove_tab(int index) { (void)index; }

/* Find & replace pane functions */
const char *get_find_text(void) { return find_text; }
const char *get_repl_text(void) { return repl_text; }
void set_find_text(const char *text) {
	if (text) {
		snprintf(find_text, sizeof(find_text), "%s", text);
	} else {
		find_text[0] = '\0';
	}
}
void set_repl_text(const char *text) {
	if (text) {
		snprintf(repl_text, sizeof(repl_text), "%s", text);
	} else {
		repl_text[0] = '\0';
	}
}
void add_to_find_history(const char *text) { (void)text; }
void add_to_repl_history(const char *text) { (void)text; }
void set_entry_font(const char *name) { (void)name; }
bool is_checked(FindOption *option) { (void)option; return false; }
void toggle(FindOption *option, bool on) { (void)option; (void)on; }
void set_find_label(const char *text) { (void)text; }
void set_repl_label(const char *text) { (void)text; }
void set_button_label(FindButton *button, const char *text) { (void)button; (void)text; }
void set_option_label(FindOption *option, const char *text) { (void)option; (void)text; }
void focus_find(void) {}

/* Command entry functions */
void focus_command_entry(void) {
	command_entry_active = !command_entry_active;
	/* TODO: show/hide command entry UI */
}
bool is_command_entry_active(void) { return command_entry_active; }
void set_command_entry_label(const char *text) {
	/* placeholder for command entry label */
	(void)text;
}
int get_command_entry_height(void) { return 1; }
void set_command_entry_height(int height) { (void)height; }

/* Statusbar functions */
bool is_statusbar_visible(void) { return statusbar_visible; }
void set_statusbar_visible(bool visible) { statusbar_visible = visible; }
const char *get_statusbar_text(int bar) {
	if (bar == 0) return statusbar_text0;
	if (bar == 1) return statusbar_text1;
	return "";
}
void set_statusbar_text(int bar, const char *text) {
	if (bar == 0 && text) snprintf(statusbar_text0, sizeof(statusbar_text0), "%s", text);
	else if (bar == 1 && text) snprintf(statusbar_text1, sizeof(statusbar_text1), "%s", text);
}

/* Menu functions */
void *read_menu(lua_State *L, int index) { (void)L; (void)index; return NULL; }
void popup_menu(void *menu, void *userdata) {
	(void)menu; (void)userdata;
	// Notcurses popup menu not implemented yet.
	fprintf(stderr, "popup_menu called (menu=%p)\n", menu);
}
void set_menubar(lua_State *L, int index) { (void)L; (void)index; }

char *get_clipboard_text(int *len) {
	/* TODO: implement clipboard access via notcurses (or OS‑specific fallback) */
	(void)len;
	return NULL;
}

void add_timeout(double interval, bool (*f)(int *), int *reference) {
	(void)interval; (void)f; (void)reference;
}

void update_ui(void) {
	if (!ensure_notcurses()) return;
	notcurses_render(nc);
}

bool is_hidpi(void) { return false; }
bool is_dark_mode(void) { return false; }

int message_dialog(DialogOptions opts, lua_State *L) {
	if (!ensure_notcurses()) return 0;
	struct ncplane* std = notcurses_stdplane(nc);
	int rows, cols;
	ncplane_dim_yx(std, &rows, &cols);
	// Tamanho do diálogo
	int h = 8;
	int w = cols * 2 / 3;
	if (w > 70) w = 70;
	int y = (rows - h) / 2;
	int x = (cols - w) / 2;

	struct ncplane_options popt = {
		.y = y,
		.x = x,
		.rows = h,
		.cols = w,
		.userptr = NULL,
		.name = "dialog",
		.resizecb = NULL,
		.flags = 0,
	};
	struct ncplane* dplane = ncplane_create(std, &popt);
	if (!dplane) return 0;

	// Preencher fundo
	ncplane_set_base(dplane, " ", 0, 0);
	// Título
	if (opts.title) {
		int titlex = (w - (int)strlen(opts.title)) / 2;
		if (titlex < 0) titlex = 0;
		ncplane_putstr_yx(dplane, 1, titlex, "%s", opts.title);
	}
	// Texto (primeira linha apenas, por simplicidade)
	if (opts.text) {
		char line[256];
		snprintf(line, sizeof(line), "%.*s", w - 4, opts.text);
		ncplane_putstr_yx(dplane, 3, 2, "%s", line);
	}
	// Botões (usa o primeiro botão fornecido ou "OK")
	const char* btn = opts.buttons[0] ? opts.buttons[0] : "OK";
	int btnx = (w - (int)strlen(btn)) / 2;
	ncplane_putstr_yx(dplane, h - 3, btnx, "[ %s ]", btn);
	notcurses_render(nc);

	// Esperar uma tecla (qualquer uma)
	struct ncinput ni;
	notcurses_get_blocking(nc, &ni);
	// Destruir plano do diálogo
	ncplane_destroy(dplane);
	notcurses_render(nc);

	// Retorna índice 1 (primeiro botão)
	lua_pushinteger(L, 1);
	return 1;
}
int input_dialog(DialogOptions opts, lua_State *L) {
    if (!ensure_notcurses()) return 0;
    struct ncplane* std = notcurses_stdplane(nc);
    int rows, cols;
    ncplane_dim_yx(std, &rows, &cols);
    int h = 10;
    int w = cols * 2 / 3;
    if (w > 70) w = 70;
    int y = (rows - h) / 2;
    int x = (cols - w) / 2;

    struct ncplane_options popt = {
        .y = y,
        .x = x,
        .rows = h,
        .cols = w,
        .userptr = NULL,
        .name = "input_dialog",
        .resizecb = NULL,
        .flags = 0,
    };
    struct ncplane* dplane = ncplane_create(std, &popt);
    if (!dplane) return 0;
    ncplane_set_base(dplane, " ", 0, 0);
    // Título
    if (opts.title) {
        int titlex = (w - (int)strlen(opts.title)) / 2;
        if (titlex < 0) titlex = 0;
        ncplane_putstr_yx(dplane, 1, titlex, "%s", opts.title);
    }
    // Texto do prompt
    if (opts.text) {
        ncplane_putstr_yx(dplane, 3, 2, "%s", opts.text);
    }
    // Campo de entrada com borda
    int input_y = 5;
    int input_x = 2;
    int input_w = w - 4;
    // Desenhar borda superior
    ncplane_putstr_yx(dplane, input_y, input_x, "┌");
    for (int i = 0; i < input_w-2; i++) ncplane_putstr_yx(dplane, input_y, input_x+1+i, "─");
    ncplane_putstr_yx(dplane, input_y, input_x+input_w-1, "┐");
    // Lados
    ncplane_putstr_yx(dplane, input_y+1, input_x, "│");
    ncplane_putstr_yx(dplane, input_y+1, input_x+input_w-1, "│");
    // Borda inferior
    ncplane_putstr_yx(dplane, input_y+2, input_x, "└");
    for (int i = 0; i < input_w-2; i++) ncplane_putstr_yx(dplane, input_y+2, input_x+1+i, "─");
    ncplane_putstr_yx(dplane, input_y+2, input_x+input_w-1, "┘");
    // Área de entrada interna
    int text_x = input_x + 1;
    int text_y = input_y + 1;
    int max_len = input_w - 2;
    char *buf = malloc(max_len + 1);
    if (!buf) { ncplane_destroy(dplane); return 0; }
    memset(buf, 0, max_len + 1);
    int curpos = 0;
    int offset = 0;

    // Botões
    const char* btn_ok = opts.buttons[0] ? opts.buttons[0] : "OK";
    const char* btn_cancel = opts.buttons[1] ? opts.buttons[1] : "Cancel";
    int btn_ok_len = strlen(btn_ok);
    int btn_cancel_len = strlen(btn_cancel);
    int btn_y = h - 3;
    int btn_ok_x = (w - (btn_ok_len + btn_cancel_len + 4)) / 2;
    int btn_cancel_x = btn_ok_x + btn_ok_len + 2;

    bool done = false;
    bool accepted = false;
    int ret_button = 1; // 1 para primeiro botão (OK), 2 para segundo (Cancel)
    struct ncinput ni;
    while (!done) {
        // Limpar área dos botões
        ncplane_putstr_yx(dplane, btn_y, btn_ok_x-1, "   ");
        ncplane_putstr_yx(dplane, btn_y, btn_ok_x, "%s", btn_ok);
        ncplane_putstr_yx(dplane, btn_y, btn_cancel_x-1, "   ");
        ncplane_putstr_yx(dplane, btn_y, btn_cancel_x, "%s", btn_cancel);
        // Destacar botão selecionado
        if (ret_button == 1) {
            ncplane_putstr_yx(dplane, btn_y, btn_ok_x-1, "[");
            ncplane_putstr_yx(dplane, btn_y, btn_ok_x+btn_ok_len, "]");
        } else {
            ncplane_putstr_yx(dplane, btn_y, btn_cancel_x-1, "[");
            ncplane_putstr_yx(dplane, btn_y, btn_cancel_x+btn_cancel_len, "]");
        }
        // Atualizar texto visível
        char visible[max_len+1];
        // Ajustar offset para manter cursor visível
        if (curpos - offset >= max_len) offset = curpos - max_len + 1;
        if (offset > curpos) offset = curpos;
        int copy_len = max_len;
        if (offset + copy_len > (int)strlen(buf)) copy_len = strlen(buf) - offset;
        if (copy_len < 0) copy_len = 0;
        strncpy(visible, buf + offset, copy_len);
        visible[copy_len] = '\0';
        ncplane_putstr_yx(dplane, text_y, text_x, "%-*s", max_len, visible);
        // Posicionar cursor
        int cursor_scr = curpos - offset;
        ncplane_cursor_move_yx(dplane, text_y, text_x + cursor_scr);

        notcurses_render(nc);

        notcurses_get_blocking(nc, &ni);
        if (ni.evtype == NCTYPE_PRESS) {
            if (ni.id == NCKEY_ENTER || ni.id == '\n' || ni.id == '\r') {
                accepted = (ret_button == 1);
                done = true;
            } else if (ni.id == NCKEY_ESC) {
                accepted = false;
                done = true;
            } else if (ni.id == NCKEY_TAB) {
                ret_button = (ret_button == 1) ? 2 : 1;
            } else if (ni.id == NCKEY_LEFT) {
                if (curpos > 0) curpos--;
            } else if (ni.id == NCKEY_RIGHT) {
                if (curpos < (int)strlen(buf)) curpos++;
            } else if (ni.id == NCKEY_BACKSPACE || ni.id == NCKEY_DEL) {
                if (curpos > 0) {
                    memmove(buf + curpos - 1, buf + curpos, strlen(buf) - curpos + 1);
                    curpos--;
                }
            } else if (ni.id == NCKEY_HOME) {
                curpos = 0;
            } else if (ni.id == NCKEY_END) {
                curpos = strlen(buf);
            } else if (isprint((unsigned char)ni.id) && strlen(buf) < max_len) {
                memmove(buf + curpos + 1, buf + curpos, strlen(buf) - curpos + 1);
                buf[curpos] = (char)ni.id;
                curpos++;
            }
        }
    }

    ncplane_destroy(dplane);
    notcurses_render(nc);

    if (accepted) {
        lua_pushstring(L, buf);
        if (opts.return_button) {
            lua_pushinteger(L, ret_button);
            free(buf);
            return 2;
        }
        free(buf);
        return 1;
    } else {
        free(buf);
        return 0;
    }
}
int open_dialog(DialogOptions opts, lua_State *L) { (void)opts; (void)L; return 0; }
int save_dialog(DialogOptions opts, lua_State *L) { (void)opts; (void)L; return 0; }
int progress_dialog(DialogOptions opts, lua_State *L,
	bool (*work)(void (*update)(double percent, const char *text, void *userdata), void *userdata)) {
	(void)opts; (void)L; (void)work; return 0;
}
int list_dialog(DialogOptions opts, lua_State *L) { (void)opts; (void)L; return 0; }

bool spawn(lua_State *L, Process *proc, int index, const char *cmd, const char *cwd, int envi,
	bool monitor_stdout, bool monitor_stderr, const char **error) {
	(void)L; (void)proc; (void)index; (void)cmd; (void)cwd; (void)envi;
	(void)monitor_stdout; (void)monitor_stderr; (void)error;
	return false;
}
size_t process_size(void) { return 0; }
bool is_process_running(Process *proc) { (void)proc; return false; }
void wait_process(Process *proc) { (void)proc; }
char *read_process_output(Process *proc, char option, size_t *len, const char **error, int *code) {
	(void)proc; (void)option; (void)len; (void)error; (void)code;
	return NULL;
}
void write_process_input(Process *proc, const char *s, size_t len) { (void)proc; (void)s; (void)len; }
void close_process_input(Process *proc) { (void)proc; }
void kill_process(Process *proc, int signal) { (void)proc; (void)signal; }
int get_process_exit_status(Process *proc) { (void)proc; return 0; }
void cleanup_process(Process *proc) { (void)proc; }
void suspend(void) {}
void quit(void) {}

/* ------------------------------------------------------------------------ */

