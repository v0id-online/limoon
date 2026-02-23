# Makefile for Textadept with Notcurses frontend
#
# This file builds the textadept-notcurses executable.
#

# Compiler and flags
CC      = gcc
CFLAGS  = -I. -I./src -I./core -std=c99 -Wall -Wextra -pedantic -D_GNU_SOURCE
LDFLAGS =
LIBS    = -lnotcurses -lscintilla -llua -lm -lutil -lpanelw -lncursesw

# Source directories
SRCDIR  = src
COREDIR = core

# Find all C source files in src/ and core/, excluding other frontends
CORE_SRCS = $(shell find $(SRCDIR) -name "*.c" ! -name "*_curses.c" ! -name "*_gtk.c" ! -name "*_qt.c" ! -name "n_textadept.c") \
            $(shell find $(COREDIR) -name "*.c" 2>/dev/null || true)

# The Notcurses frontend source
N_SRCS = $(SRCDIR)/n_textadept.c

# Object files
CORE_OBJS = $(CORE_SRCS:.c=.o)
N_OBJS    = $(N_SRCS:.c=.o)

# Executable name
TARGET = textadept-notcurses

# Default target
all: $(TARGET)

# Linking the final executable
$(TARGET): $(CORE_OBJS) $(N_OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

# Compile core .c files
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Clean up
clean:
	rm -f $(CORE_OBJS) $(N_OBJS) $(TARGET)

.PHONY: all clean
