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


static bool ensure_notcurses(void);
static View* create_view(void);
static void destroy_view(View *view);
static void update_view(View *view, int y, int x, const char *str);

/* ------------------------------------------------------------------------ */

int main(int argc, char **argv) {
	fprintf(stderr, "[n_textadept] main starting\n");
	/* Initialize Notcurses */
	if (!ensure_notcurses()) {
		fprintf(stderr, "[n_textadept] Failed to initialize Notcurses\n");
		return 1;
	}

	fprintf(stderr, "[n_textadept] calling init_textadept\n");
	/* Call init_textadept which will call new_window etc. */
	if (!init_textadept(argc, argv)) {
		fprintf(stderr, "[n_textadept] Failed to initialize Textadept\n");
		notcurses_stop(nc);
		return 1;
	}

	struct ncinput ni;
	bool running = true;
	fprintf(stderr, "[n_textadept] entering main loop\n");
	while (running && !want_quit) {
		/* Process any pending UI updates */
		update_ui();
		/* Non-blocking input */
		while (notcurses_get_nblock(nc, &ni) != -1) {
			if (ni.evtype == NCTYPE_PRESS) {
				fprintf(stderr, "[n_textadept] key press: id=%d, modifiers=%x\n",
				        ni.id, ni.modifiers);
				/* For now, exit on 'q' or Ctrl+C */
				if (ni.id == 'q' || (ni.id == 'c' && (ni.modifiers & NCTRL_MSK))) {
					fprintf(stderr, "[n_textadept] quitting\n");
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

	fprintf(stderr, "[n_textadept] cleaning up\n");
	/* Cleanup */
	close_textadept();
	notcurses_stop(nc);
	fprintf(stderr, "[n_textadept] exiting with status %d\n", exit_status);
	/* Avoid returning uninitialized exit_status if something went wrong */
	if (exit_status != 0) {
		return exit_status;
	}
	return 0;
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
static bool want_quit = false;

static bool ensure_notcurses(void) {
	if (!nc) {
		fprintf(stderr, "[n_textadept] initializing notcurses\n");
		nc = notcurses_init(NULL, stdout);
		if (!nc) {
			fprintf(stderr, "[n_textadept] failed to initialize notcurses\n");
			return false;
		}
		fprintf(stderr, "[n_textadept] notcurses initialized successfully\n");
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
	fprintf(stderr, "[n_textadept] new_window called\n");
	if (!ensure_notcurses()) return;
	current_view = create_view();
	if (!current_view) {
		fprintf(stderr, "[n_textadept] failed to create view\n");
		return;
	}
	fprintf(stderr, "[n_textadept] view created, calling get_view\n");
	SciObject *sci = get_view();
	if (sci) {
		current_view->sci = sci;
		fprintf(stderr, "[n_textadept] scintilla view associated\n");
		focus_view(sci);
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
	fprintf(stderr, "[n_textadept] focus_view called (view=%p)\n", view);
	if (!view) {
		fprintf(stderr, "[n_textadept] WARNING: view is NULL\n");
		return;
	}
	if (focused_view && focused_view != view) {
		fprintf(stderr, "[n_textadept] unfocusing previous view %p\n", focused_view);
		SS(focused_view, SCI_SETFOCUS, 0, 0);
	}
	SS(view, SCI_SETFOCUS, 1, 0);
	focused_view = view;
	fprintf(stderr, "[n_textadept] focused view set to %p\n", view);
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
Pane *get_top_pane(void) {
	fprintf(stderr, "[n_textadept] get_top_pane called\n");
	return NULL;
}
PaneInfo get_pane_info(Pane *pane) {
	fprintf(stderr, "[n_textadept] get_pane_info called (pane=%p)\n", pane);
	(void)pane; PaneInfo info = {0}; return info;
}
PaneInfo get_parent_pane_info(PaneInfo info) {
	fprintf(stderr, "[n_textadept] get_parent_pane_info called\n");
	(void)info; PaneInfo ret = {0}; return ret;
}
PaneInfo get_pane_info_from_view(SciObject *view) {
	fprintf(stderr, "[n_textadept] get_pane_info_from_view called (view=%p)\n", view);
	(void)view; PaneInfo info = {0}; return info;
}
void set_pane_split_pos(Pane *pane, int pos) {
	fprintf(stderr, "[n_textadept] set_pane_split_pos called (pane=%p, pos=%d)\n", pane, pos);
	(void)pane; (void)pos;
}

/* Tab functions */
void show_tabs(bool show) {
	fprintf(stderr, "[n_textadept] show_tabs called (%s)\n", show ? "true" : "false");
	(void)show;
}
void add_tab(void) {
	fprintf(stderr, "[n_textadept] add_tab called\n");
}
void set_tab(int index) {
	fprintf(stderr, "[n_textadept] set_tab called (index=%d)\n", index);
	(void)index;
}
void set_tab_label(int index, const char *text) {
	fprintf(stderr, "[n_textadept] set_tab_label called (index=%d, text=%s)\n", index, text ? text : "NULL");
	(void)index; (void)text;
}
void move_tab(int from, int to) {
	fprintf(stderr, "[n_textadept] move_tab called (from=%d, to=%d)\n", from, to);
	(void)from; (void)to;
}
void remove_tab(int index) {
	fprintf(stderr, "[n_textadept] remove_tab called (index=%d)\n", index);
	(void)index;
}

/* Find & replace pane functions */
const char *get_find_text(void) {
	fprintf(stderr, "[n_textadept] get_find_text called -> '%s'\n", find_text);
	return find_text;
}
const char *get_repl_text(void) {
	fprintf(stderr, "[n_textadept] get_repl_text called -> '%s'\n", repl_text);
	return repl_text;
}
void set_find_text(const char *text) {
	fprintf(stderr, "[n_textadept] set_find_text called (text=%s)\n", text ? text : "NULL");
	if (text) {
		snprintf(find_text, sizeof(find_text), "%s", text);
	} else {
		find_text[0] = '\0';
	}
}
void set_repl_text(const char *text) {
	fprintf(stderr, "[n_textadept] set_repl_text called (text=%s)\n", text ? text : "NULL");
	if (text) {
		snprintf(repl_text, sizeof(repl_text), "%s", text);
	} else {
		repl_text[0] = '\0';
	}
}
void add_to_find_history(const char *text) {
	fprintf(stderr, "[n_textadept] add_to_find_history called (text=%s)\n", text ? text : "NULL");
	(void)text;
}
void add_to_repl_history(const char *text) {
	fprintf(stderr, "[n_textadept] add_to_repl_history called (text=%s)\n", text ? text : "NULL");
	(void)text;
}
void set_entry_font(const char *name) {
	fprintf(stderr, "[n_textadept] set_entry_font called (name=%s)\n", name ? name : "NULL");
	(void)name;
}
bool is_checked(FindOption *option) {
	fprintf(stderr, "[n_textadept] is_checked called (option=%p)\n", option);
	(void)option; return false;
}
void toggle(FindOption *option, bool on) {
	fprintf(stderr, "[n_textadept] toggle called (option=%p, on=%s)\n", option, on ? "true" : "false");
	(void)option; (void)on;
}
void set_find_label(const char *text) {
	fprintf(stderr, "[n_textadept] set_find_label called (text=%s)\n", text ? text : "NULL");
	(void)text;
}
void set_repl_label(const char *text) {
	fprintf(stderr, "[n_textadept] set_repl_label called (text=%s)\n", text ? text : "NULL");
	(void)text;
}
void set_button_label(FindButton *button, const char *text) {
	fprintf(stderr, "[n_textadept] set_button_label called (button=%p, text=%s)\n", button, text ? text : "NULL");
	(void)button; (void)text;
}
void set_option_label(FindOption *option, const char *text) {
	fprintf(stderr, "[n_textadept] set_option_label called (option=%p, text=%s)\n", option, text ? text : "NULL");
	(void)option; (void)text;
}
void focus_find(void) {
	fprintf(stderr, "[n_textadept] focus_find called\n");
}

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
int get_command_entry_height(void) { return command_entry_active ? 1 : 0; }
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
	// implementação stub: tenta ler da clipboard do terminal via OSC 52 (limitado)
	// apenas retorna NULL por enquanto.
	(void)len;
	return NULL;
}

void add_timeout(double interval, bool (*f)(int *), int *reference) {
	// implementação stub: armazena em uma lista para processamento futuro.
	// por simplicidade, ignoramos por enquanto.
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
        // Garantir que offset não esteja além do fim do buffer
        size_t buflen = strlen(buf);
        if (offset > (int)buflen) offset = (int)buflen;
        if (offset < 0) offset = 0;
        int copy_len = max_len;
        if (offset + copy_len > (int)buflen) copy_len = (int)buflen - offset;
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
int open_dialog(DialogOptions opts, lua_State *L) {
	DialogOptions io_opts = opts;
	if (!io_opts.title) io_opts.title = "Open File";
	if (!io_opts.text) io_opts.text = "Enter file path:";
	// reuse input_dialog
	return input_dialog(io_opts, L);
}
int save_dialog(DialogOptions opts, lua_State *L) {
	DialogOptions io_opts = opts;
	if (!io_opts.title) io_opts.title = "Save File";
	if (!io_opts.text) io_opts.text = "Enter file path:";
	return input_dialog(io_opts, L);
}
int progress_dialog(DialogOptions opts, lua_State *L,
	bool (*work)(void (*update)(double percent, const char *text, void *userdata), void *userdata)) {
	if (!ensure_notcurses()) return 0;
	struct ncplane* std = notcurses_stdplane(nc);
	int rows, cols;
	ncplane_dim_yx(std, &rows, &cols);
	int h = 10;
	int w = cols * 2 / 3;
	if (w > 60) w = 60;
	int y = (rows - h) / 2;
	int x = (cols - w) / 2;
	struct ncplane_options popt = {
		.y = y, .x = x, .rows = h, .cols = w,
		.userptr = NULL, .name = "progress", .resizecb = NULL, .flags = 0,
	};
	struct ncplane* dplane = ncplane_create(std, &popt);
	if (!dplane) return 0;
	ncplane_set_base(dplane, " ", 0, 0);
	if (opts.title) {
		int titlex = (w - (int)strlen(opts.title)) / 2;
		if (titlex < 0) titlex = 0;
		ncplane_putstr_yx(dplane, 1, titlex, "%s", opts.title);
	}
	if (opts.text) {
		ncplane_putstr_yx(dplane, 3, 2, "%s", opts.text);
	}
	// barra de progresso
	int bar_y = 5;
	int bar_x = 2;
	int bar_w = w - 4;
	ncplane_putstr_yx(dplane, bar_y, bar_x, "[");
	ncplane_putstr_yx(dplane, bar_y, bar_x + bar_w - 1, "]");
	for (int i = 1; i < bar_w - 1; i++)
		ncplane_putstr_yx(dplane, bar_y, bar_x + i, " ");
	bool work_result = false;
	bool cancelled = false;
	struct ncinput ni;
	// variáveis para o callback update
	double current_percent = 0.0;
	const char* current_text = "";
	void* userdata = NULL; // não usado
	// função update local
	void update(double percent, const char* text, void* udata) {
		(void)udata;
		current_percent = percent;
		current_text = text ? text : "";
		// redesenhar barra
		int filled = (int)((bar_w - 2) * percent / 100.0);
		if (filled < 0) filled = 0;
		if (filled > bar_w - 2) filled = bar_w - 2;
		for (int i = 1; i < bar_w - 1; i++) {
			if (i <= filled)
				ncplane_putstr_yx(dplane, bar_y, bar_x + i, "=");
			else
				ncplane_putstr_yx(dplane, bar_y, bar_x + i, " ");
		}
		// texto
		char line[256];
		snprintf(line, sizeof(line), "%.0f%% %s", percent, current_text);
		ncplane_putstr_yx(dplane, bar_y + 2, bar_x, "%-*s", bar_w, line);
		notcurses_render(nc);
		// processar eventos para permitir cancelamento (ESC)
		while (notcurses_get_nblock(nc, &ni) != -1) {
			if (ni.evtype == NCTYPE_PRESS && ni.id == NCKEY_ESC) {
				cancelled = true;
				break;
			}
		}
	}
	// chamar work em um loop (a própria work deve controlar o fim)
	work_result = work(update, userdata);
	// limpar
	ncplane_destroy(dplane);
	notcurses_render(nc);
	if (cancelled) {
		lua_pushboolean(L, true);
		return 1;
	}
	return 0;
}
int list_dialog(DialogOptions opts, lua_State *L) {
	// stub simplificado: retorna cancelado
	(void)opts; (void)L;
	return 0;
}

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
void suspend(void) {
	// Não há suporte a suspensão em Notcurses; apenas stub.
}
void quit(void) {
    want_quit = true;
}

/* ------------------------------------------------------------------------ */

