# Makefile for Li Moon — convenience wrapper around CMake.
#
# Quick start:
#   make          — configure + build (first run fetches deps, takes a while)
#   make run      — run li from the build dir with LIMOON_HOME set
#   make clean    — remove build/
#   make install  — install to /usr/local (or PREFIX=...)

BUILD       = ./build
LIMOON_HOME = $(CURDIR)
TARGET      = $(BUILD)/li

.PHONY: all run install clean dev-setup

all: dev-setup
	cmake -S . -B $(BUILD)
	cmake --build $(BUILD) --parallel
	@cp $(BUILD)/li ./li 2>/dev/null || true

run: dev-setup $(TARGET)
	LIMOON_HOME=$(LIMOON_HOME) $(TARGET) $(ARGS)

$(TARGET):
	$(MAKE) all

# Create lexers symlink for running from the source tree.
dev-setup: lexers

lexers:
	@if [ ! -e lexers ]; then \
	  if [ -d $(BUILD)/_deps/scintillua-src/lexers ]; then \
	    ln -s $(BUILD)/_deps/scintillua-src/lexers lexers && \
	    echo "Created lexers -> $(BUILD)/_deps/scintillua-src/lexers"; \
	  else \
	    echo "Lexers not found — run 'make all' first to fetch deps"; \
	  fi \
	fi

install: $(TARGET)
	cmake --install $(BUILD) $(if $(PREFIX),--prefix $(PREFIX),)

clean:
	rm -rf $(BUILD) lexers
