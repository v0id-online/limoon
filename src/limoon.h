// Copyright 2007-2026 Mitchell. See LICENSE.
// Interface between platforms and Li Moon.
// Platforms use this interface to communicate with Li Moon.
#ifndef LIMOON_H
#define LIMOON_H

#include "limoon_platform.h"

// Li Moon's home directory.
extern char *limoon_home;

// The currently focused view and command entry.
extern SciObject *focused_view, *command_entry;

// Find & replace pane buttons and options.
extern FindButton *find_next, *find_prev, *replace, *replace_all;
extern FindOption *match_case, *whole_word, *regex, *in_files;

// Li Moon's Lua state. Platforms should generally refrain from modifying it, but access may
// be occasionally needed.
extern lua_State *lua;

// Li Moon's exit status. Platforms should return it from their main() functions.
extern int exit_status;

/** Initializes Li Moon.
 * Initializes Lua, asks the Platform to create the main application window, and runs Lua
 * startup scripts. Platforms should typically call this after their own initialization and
 * before starting the main event loop.
 * Any startup errors are presented in a dialog. Emits an 'initialized' event on success.
 * @param argc The number of command line arguments.
 * @param argv List of command line argument strings.
 * @return whether or not initialization succeeded
 */
bool init_limoon(int argc, char **argv);

/** Emits a Lua event.
 * @param name The event name.
 * @param ... Arguments to pass with the event. Each pair of arguments should be a Lua type
 *	followed by the data value itself. For LUA_TLIGHTUSERDATA and LUA_TTABLE types, push
 *	the data values to the stack and give the value returned by luaL_ref(); luaL_unref()
 *	will be called appropriately. The list must be terminated with a -1.
 * @return true or false depending on the boolean value returned by the event handler, if any.
 */
bool emit(const char *name, ...);

/** Moves the buffer from the given index to another index in the 'buffers' registry table,
 * shifting other buffers as necessary.
 * @param from 1-based index of the buffer to move.
 * @param to 1-based index to move the buffer to.
 * @reorder_tabs Whether or not to reorder platform tabs. This is `false` when responding to
 *	a platform reordering event and `true` when calling from Lua.
 */
void move_buffer(int from, int to, bool reorder_tabs);

/** Signal for a find & replace pane button click.
 * Emits 'find', 'replace', and/or 'replace_all' events depending on the button clicked.
 * @param button The button clicked.
 */
void find_clicked(FindButton *button);

/** Requests to show a context menu.
 * Li Moon will lookup that menu and call `popup_menu()` in turn.
 * @param name The name of the context menu, either "context_menu" or "tab_context_menu".
 * @param userdata Userdata to pass to `popup_menu()`.
 * @see popup_menu
 * @see read_menu
 */
void show_context_menu(const char *name, void *userdata);

/** Notifies Li Moon that there has been a change between light mode and dark mode.
 * Li Moon will call `is_dark_mode()` in order to determine which mode is enabled.
 */
void mode_changed(void);

/** Notifies Li Moon that a spawned child process has produced the given stdout or stderr.
 * Li Moon will call any functions listening for that output.
 */
void process_output(Process *proc, const char *s, size_t len, bool is_stdout);

/** Notifies Li Moon that a spawned child process has exited with the given exit code.
 * Li Moon will call an exit function (if it exists) for that process.
 */
void process_exited(Process *proc, int code);

/** Asks Li Moon if the platform can quit.
 * If the return value is `false`, something is preventing Li Moon from quitting (e.g. unsaved
 * changes) and the platform should not quit yet.
 * @return true or false depending on whether Li Moon is ready to quit
 */
bool can_quit(void);

/** Closes Li Moon.
 * Unsplits panes, closes buffers, deletes Scintilla views, and closes Lua. During this process,
 * Li Moon may still call `SS()`, so platforms should take care to call this while Scintilla
 * is still available (perhaps just before exiting the main event loop).
 * This does not need to be called if `init_limoon()` failed.
 */
void close_limoon(void);

/** Returns the value t[n] as an integer where t is the value at the given valid index.
 * The access is raw; that is, it does not invoke metamethods.
 * This is a helper function for easily reading integers from lists.
 * @param L The Lua state.
 * @param index The stack index of the table.
 * @param n The index in the table to get.
 * @return integer
 */
int get_int_field(lua_State *L, int index, int n);

#endif /* LIMOON_H */
