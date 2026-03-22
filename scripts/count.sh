#!/bin/bash
# Copyright 2022-2026 Mitchell. See LICENSE.

# Counts lines of code for the each platform.
# Requires cloc.

files="core modules/limoon src/limoon.c src/limoon.h src/limoon_platform.h \
	CMakeLists.txt init.lua"
opts="--exclude-lang=SVG --force-lang=C,h --not-match-f=_test --quiet"

cd ..
echo -n === Gtk ===
cloc $files src/limoon_gtk.c $opts
echo -n === Curses ===
cloc $files src/limoon_curses.c $opts
echo -n === Qt ===
cloc $files src/limoon_qt.cpp src/limoon_qt.h $opts
