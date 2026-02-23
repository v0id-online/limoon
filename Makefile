# Makefile for Textadept with Notcurses frontend
#
# This file builds the textadept-notcurses executable.
#

# Compiler and flags
CC      = gcc
CFLAGS  = -I. -I./src -I./core -std=c99 -Wall -Wextra -pedantic -D_GNU_SOURCE
LDFLAGS =
LIBS    = -lnotcurses -lscintilla -llua -lm -lutil -lpanelw -lncursesw

# Try to use pkg-config for notcurses and scintilla
PKG_CONFIG = pkg-config

# Notcurses flags
ifneq ($(shell $(PKG_CONFIG) --exists notcurses && echo yes),)
CFLAGS  += $(shell $(PKG_CONFIG) --cflags notcurses)
LIBS    := $(shell $(PKG_CONFIG) --libs notcurses) -lscintilla -llua -lm -lutil -lpanelw -lncursesw
else
$(warning "pkg-config notcurses not found, using default flags")
endif

# Scintilla flags (try pkg-config, then find header location)
ifneq ($(shell $(PKG_CONFIG) --exists scintilla && echo yes),)
CFLAGS  += $(shell $(PKG_CONFIG) --cflags scintilla)
LIBS    := $(filter-out -lscintilla,$(LIBS)) $(shell $(PKG_CONFIG) --libs scintilla) $(filter -l%,$(LIBS))
else
# Try to find Scintilla.h using find command and take the first result's parent directory
SCINTILLA_HEADER := $(shell find /usr/include /usr/local/include -name "Scintilla.h" 2>/dev/null | head -1)
ifneq ($(SCINTILLA_HEADER),)
SCINTILLA_INCLUDE := $(patsubst %/Scintilla.h,%,$(SCINTILLA_HEADER))
CFLAGS += -I$(SCINTILLA_INCLUDE)
else
$(warning "Scintilla.h not found in standard locations, trying common paths")
SCINTILLA_INCLUDES := -I/usr/include/scintilla -I/usr/local/include/scintilla -I/usr/include -I/usr/local/include -I/usr/include/scintilla/include
CFLAGS  += $(SCINTILLA_INCLUDES)
endif
endif

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
	@echo "Build completed. Run './$(TARGET)' to start."

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
