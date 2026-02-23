# Makefile for Textadept with Notcurses frontend
#
# This file builds the textadept-notcurses executable.
#

# Compiler and flags
CC      = gcc
CFLAGS  = -I. -I./src -I./core -std=c99 -Wall -Wextra -pedantic -D_GNU_SOURCE
LDFLAGS =
LIBS    = -lnotcurses -lscintilla -lscintillua -llua -lm -lutil -lpanelw -lncursesw

# Try to use pkg-config for notcurses and scintilla
PKG_CONFIG = pkg-config

# Notcurses flags
ifneq ($(shell $(PKG_CONFIG) --exists notcurses && echo yes),)
CFLAGS  += $(shell $(PKG_CONFIG) --cflags notcurses)
LIBS    := $(shell $(PKG_CONFIG) --libs notcurses) -lscintilla -lscintillua -llua -lm -lutil -lpanelw -lncursesw
else
$(warning "pkg-config notcurses not found, using default flags")
endif

# Scintilla flags (try pkg-config for scintilla and qscintilla, then find header location)
SCINTILLA_FOUND := no

# First, try pkg-config for scintilla (standalone)
ifneq ($(shell $(PKG_CONFIG) --exists scintilla && echo yes),)
CFLAGS  += $(shell $(PKG_CONFIG) --cflags scintilla)
LIBS    := $(filter-out -lscintilla,$(LIBS)) $(shell $(PKG_CONFIG) --libs scintilla) $(filter -l%,$(LIBS))
SCINTILLA_FOUND := yes
endif

# If not found, try pkg-config for qscintilla2-qt6 (Fedora)
ifeq ($(SCINTILLA_FOUND),no)
ifneq ($(shell $(PKG_CONFIG) --exists qscintilla2-qt6 && echo yes),)
CFLAGS  += $(shell $(PKG_CONFIG) --cflags qscintilla2-qt6)
LIBS    := $(filter-out -lscintilla,$(LIBS)) $(shell $(PKG_CONFIG) --libs qscintilla2-qt6) $(filter -l%,$(LIBS))
SCINTILLA_FOUND := yes
# Define macro so that we can include <Qsci/qscintilla.h> instead
CFLAGS  += -DHAVE_QSCI_QSCINTILLA_H
endif
endif

# If still not found, try pkg-config for qscintilla2-qt5
ifeq ($(SCINTILLA_FOUND),no)
ifneq ($(shell $(PKG_CONFIG) --exists qscintilla2-qt5 && echo yes),)
CFLAGS  += $(shell $(PKG_CONFIG) --cflags qscintilla2-qt5)
LIBS    := $(filter-out -lscintilla,$(LIBS)) $(shell $(PKG_CONFIG) --libs qscintilla2-qt5) $(filter -l%,$(LIBS))
SCINTILLA_FOUND := yes
CFLAGS  += -DHAVE_QSCI_QSCINTILLA_H
endif
endif

# If still not found, search for header in common locations
ifeq ($(SCINTILLA_FOUND),no)
# Try to find Scintilla.h or qscintilla.h using find command
SCINTILLA_HEADER := $(shell find /usr/include /usr/local/include -name "Scintilla.h" -o -name "qscintilla.h" 2>/dev/null | head -1)
ifneq ($(SCINTILLA_HEADER),)
SCINTILLA_INCLUDE := $(patsubst %/Scintilla.h,%,$(SCINTILLA_HEADER))
SCINTILLA_INCLUDE := $(patsubst %/qscintilla.h,%,$(SCINTILLA_INCLUDE))
CFLAGS += -I$(SCINTILLA_INCLUDE)
# Determine which header we found
ifeq ($(suffix $(SCINTILLA_HEADER)),.h)
ifneq ($(findstring qscintilla,$(SCINTILLA_HEADER)),)
CFLAGS += -DHAVE_QSCI_QSCINTILLA_H
endif
endif
SCINTILLA_FOUND := yes
else
$(warning "Scintilla.h not found in standard locations, trying common paths")
SCINTILLA_INCLUDES := -I/usr/include/scintilla -I/usr/local/include/scintilla \
                      -I/usr/include -I/usr/local/include \
                      -I/usr/include/qt6 -I/usr/include/qt5 \
                      -I/usr/include/qscintilla2 -I/usr/include/Qsci
CFLAGS  += $(SCINTILLA_INCLUDES)
endif
endif

# Adiciona a biblioteca scintillua ao linker em todas as configurações
LIBS += -lscintillua

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
