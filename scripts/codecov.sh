#!/bin/bash

# Runs unit tests and shows overall C/C++ and Lua code coverage.
# Requires luacov, xterm, and gcovr.

delete_previous_coverage=

cd ..
export LIMOON_HOME=$(pwd)
if [[ ! -z "$delete_previous_coverage" ]]; then
	find build -name "*.gcda" -delete
	rm luacov.*.out
fi
cmake build -D PROFILE=1
cmake --build build -j
xvfb-run -a build/Debug/limoon -f -t
xvfb-run -a build/Debug/limoon-gtk -f -t
xvfb-run xterm -e build/Debug/limoon-curses -t
gcovr --txt | build/Debug/limoon -n -f luacov.report.out -
cmake build -U PROFILE
cmake --build build -j
