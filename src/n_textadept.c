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
#include "textadept.h"
#include "textadept_platform.h"

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
	/* TODO: create main window and call get_view() when ready */
	(void)get_view;
}

void set_title(const char *title) {
	/* TODO: set terminal title via notcurses */
	(void)title;
}

bool is_maximized(void) { return false; }
void set_maximized(bool maximize) { (void)maximize; }
void get_size(int *w, int *h) { if(w) *w = 80; if(h) *h = 24; }
void set_size(int width, int height) { (void)width; (void)height; }

SciObject *new_scintilla(void (*notified)(SciObject *, int, SCNotification *, void *)) {
	(void)notified;
	return NULL; /* TODO */
}

void focus_view(SciObject *view) {
	/* TODO: bring a particular Scintilla view to front */
	(void)view;
}

sptr_t SS(SciObject *view, int message, uptr_t wparam, sptr_t lparam) {
	(void)view; (void)message; (void)wparam; (void)lparam;
	return 0;
}

void split_view(SciObject *view, SciObject *view2, bool vertical) {
	(void)view; (void)view2; (void)vertical;
}

bool unsplit_view(SciObject *view, void (*delete_view)(SciObject *)) {
	(void)view; (void)delete_view;
	return false;
}

void delete_scintilla(SciObject *view) {
	(void)view;
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
const char *get_find_text(void) { return ""; }
const char *get_repl_text(void) { return ""; }
void set_find_text(const char *text) { (void)text; }
void set_repl_text(const char *text) { (void)text; }
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
void focus_command_entry(void) {}
bool is_command_entry_active(void) { return false; }
void set_command_entry_label(const char *text) { (void)text; }
int get_command_entry_height(void) { return 1; }
void set_command_entry_height(int height) { (void)height; }

/* Statusbar functions */
bool is_statusbar_visible(void) { return false; }
void set_statusbar_visible(bool visible) { (void)visible; }
const char *get_statusbar_text(int bar) { (void)bar; return ""; }
void set_statusbar_text(int bar, const char *text) { (void)bar; (void)text; }

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
	/* TODO: implement UI update */
}

bool is_hidpi(void) { return false; }
bool is_dark_mode(void) { return false; }

int message_dialog(DialogOptions opts, lua_State *L) { (void)opts; (void)L; return 0; }
int input_dialog(DialogOptions opts, lua_State *L) { (void)opts; (void)L; return 0; }
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

int main(int argc, char **argv)
{
	/* Initialize Notcurses */
	nc = notcurses_init(NULL, stdout);
	if (!nc)
		return 1;

	/* Create a demo view */
	current_view = create_view();
	if (!current_view) {
		notcurses_stop(nc);
		return 1;
	}

	/* Write a welcome message */
	update_view(current_view, 0, 0, "Textadept (Notcurses) - initializing...");
	notcurses_render(nc);

	/* Initialize the core Textadept engine */
	if (!init_textadept(argc, argv))
	{
		destroy_view(current_view);
		notcurses_stop(nc);
		return 1;
	}

	/* TODO: set up initial windows, input handling, and the main event loop */

	/* Simple event loop: exit on 'q' */
	struct ncinput ni;
	bool running = true;
	while (running) {
		notcurses_get_blocking(nc, &ni);
		if (ni.evtype == NCTYPE_PRESS) {
			switch (ni.id) {
			case 'q':
				running = false;
				break;
			default:
				/* Echo typed character (just for demo) */
				char buf[2] = { (char)ni.id, '\0' };
				update_view(current_view, 1, 0, "Key pressed: ");
				ncplane_putstr_yx(current_view->plane, 1, 13, "%s", buf);
				notcurses_render(nc);
				break;
			}
		}
	}

	/* Clean up */
	destroy_view(current_view);
	notcurses_stop(nc);
	return 0;
}
