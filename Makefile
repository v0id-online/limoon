# Makefile for Li Moon with Notcurses frontend
#
# Setup: git submodule update --init scinterm-notcurses
#        cmake -S scinterm-notcurses -B scinterm-notcurses/build -DBUILD_SHARED_LIBS=OFF -DBUILD_EXAMPLES=OFF -DBUILD_TESTING=OFF -DENABLE_SCINTILLUA=OFF
#        cmake --build scinterm-notcurses/build
# Then:  make

CC      = gcc
BUILD   = ./build
DEPS    = $(BUILD)/_deps

SCINTERM_NC     = ./scinterm-notcurses
SCINTERM_NC_LIB = $(SCINTERM_NC)/build/libscinterm_notcurses_static.a

CFLAGS  = -I. -I./src -I./core \
          -I$(SCINTERM_NC)/include \
          -I$(SCINTERM_NC)/scintilla/include \
          -I$(DEPS)/lua-src/src \
          -std=gnu17 -Wall -Wextra -D_GNU_SOURCE

CFLAGS  += $(shell pkg-config --cflags notcurses)

LOCAL_LIBS = $(SCINTERM_NC_LIB) \
             $(BUILD)/liblpeg.a $(BUILD)/liblfs.a $(BUILD)/libregex.a \
             $(BUILD)/liblua.a
SYS_LIBS   = $(shell pkg-config --libs notcurses) \
             -lm -lutil -lstdc++ \
             -Wl,-z,noexecstack
LIBS = $(LOCAL_LIBS) $(SYS_LIBS)

SRCDIR = src
TARGET = limoon-notcurses

CORE_SRCS = $(shell find $(SRCDIR) -name "*.c" \
              ! -name "*_curses.c" \
              ! -name "*_gtk.c" \
              ! -name "*_qt.c" \
              ! -name "n_limoon.c")
N_SRCS    = $(SRCDIR)/n_limoon.c

CORE_OBJS = $(CORE_SRCS:.c=.o)
N_OBJS    = $(N_SRCS:.c=.o)

all: $(SCINTERM_NC_LIB) $(TARGET)
	@echo "Build completed. Run './$(TARGET)' to start."

$(SCINTERM_NC_LIB):
	cmake -S $(SCINTERM_NC) -B $(SCINTERM_NC)/build \
	  -DBUILD_SHARED_LIBS=OFF -DBUILD_EXAMPLES=OFF \
	  -DBUILD_TESTING=OFF -DENABLE_SCINTILLUA=OFF
	cmake --build $(SCINTERM_NC)/build --parallel

$(TARGET): $(CORE_OBJS) $(N_OBJS)
	$(CC) -o $@ $^ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(CORE_OBJS) $(N_OBJS) $(TARGET)

clean-all: clean
	rm -rf $(SCINTERM_NC)/build

.PHONY: all clean clean-all
