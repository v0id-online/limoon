# Copyright 2022-2026 Mitchell. See LICENSE.

cmake_minimum_required(VERSION 3.22..3.31)

project(limoon LANGUAGES C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Li Moon uses notcurses as its only terminal backend.
find_package(PkgConfig REQUIRED)
pkg_check_modules(NOTCURSES REQUIRED notcurses>=3.0.0)
find_package(Threads REQUIRED)

# Dependencies.
include(FetchContent)
set(FETCHCONTENT_QUIET OFF)
set(nightlies scintillua regex) # fetch latest version if NIGHTLY is true
set(deps_dir ${CMAKE_BINARY_DIR}/_deps)
if(POLICY CMP0135) # CMake 3.24 adds DOWNLOAD_EXTRACT_TIMESTAMP to FetchContent_Declare()
	cmake_policy(SET CMP0135 NEW)
endif()
function(fetch name url)
	string(REGEX MATCH "[^/]+$" archive ${url})
	list(FIND nightlies ${name} can_use_nightly)
	if(NIGHTLY AND can_use_nightly GREATER -1)
		string(REPLACE ${archive} default.zip url ${url}) # use nightly URL instead
	elseif(EXISTS ${deps_dir}/${archive})
		set(url file://${deps_dir}/${archive}) # use local archive instead of downloading
	endif()
	set(patch ${CMAKE_SOURCE_DIR}/src/${name}.patch)
	if(EXISTS ${patch})
		set(patch_command PATCH_COMMAND patch -N -p1 < ${patch})
	endif()
	FetchContent_Declare(${name} URL ${url} ${patch_command})
	# Note: cannot FetchContent_MakeAvailable(${name}) here, as name must be a literal.
endfunction()
fetch(scintillua https://github.com/orbitalquark/scintillua/archive/599e14c4dc2e8439d5c2de3eb5ab20818543eb2d.zip)
fetch(lua https://lua.org/ftp/lua-5.5.0.tar.gz)
fetch(lpeg https://www.inf.puc-rio.br/~roberto/lpeg/lpeg-1.1.0.tar.gz)
fetch(lfs https://github.com/keplerproject/luafilesystem/archive/v1_8_0.zip)
fetch(regex https://github.com/orbitalquark/lua-std-regex/archive/1.0.zip)
FetchContent_MakeAvailable(lua lpeg lfs regex)
if(POLICY CMP0169) # CMake 3.28 adds EXCLUDE_FROM_ALL to FetchContent_MakeAvailable()
	cmake_policy(SET CMP0169 OLD)
endif()
FetchContent_Populate(scintillua) # do not import targets; only need Lua lexers
option(FETCHCONTENT_UPDATES_DISCONNECTED "Do not update deps if already set up" ON)

# scinterm-notcurses (submodule — provides Scintilla core + notcurses platform layer).
set(SCINTERM_NC_DIR ${CMAKE_SOURCE_DIR}/scinterm-notcurses)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING  OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
add_subdirectory(${SCINTERM_NC_DIR} scinterm-notcurses-build)

# Lua.
file(GLOB lua_src ${lua_SOURCE_DIR}/src/*.c)
list(FILTER lua_src EXCLUDE REGEX "(lua|luac)\.c$")
add_library(lua STATIC ${lua_src})
target_include_directories(lua PUBLIC ${lua_SOURCE_DIR}/src)
target_compile_definitions(lua PRIVATE
	$<IF:$<BOOL:${WIN32}>,LUA_BUILD_AS_DLL,$<IF:$<BOOL:${APPLE}>,LUA_USE_MACOSX,LUA_USE_LINUX>>
	$<$<CONFIG:Debug>:LUA_USE_APICHECK>)
target_link_libraries(lua PRIVATE ${CMAKE_DL_LIBS})

# LPeg.
file(GLOB lpeg_src ${lpeg_SOURCE_DIR}/*.c)
add_library(lpeg STATIC ${lpeg_src})
target_link_libraries(lpeg PRIVATE lua)

# LFS.
file(GLOB lfs_src ${lfs_SOURCE_DIR}/src/*.c)
add_library(lfs STATIC ${lfs_src})
target_link_libraries(lfs PRIVATE lua)

# Regex.
file(GLOB regex_src ${regex_SOURCE_DIR}/*.cpp)
add_library(regex STATIC ${regex_src})
target_link_libraries(regex PRIVATE lua)

set(CMAKE_ENABLE_EXPORTS ON) # allow external Lua modules to link to exe

# Li Moon core.
set(li_src src/limoon.c)
set(li_compile_opts
	$<IF:$<NOT:$<BOOL:${WIN32}>>,-pedantic -Wall -Wextra -Wno-unused-parameter
		-Wno-missing-field-initializers,/W4>
	$<$<BOOL:${PROFILE}>:-pg --coverage>
	$<$<BOOL:${LIMOON_HOME}>:-DLIMOON_HOME="${LIMOON_HOME}">)
set(li_link_opts $<$<BOOL:${PROFILE}>:--coverage>)
# scintilla is bundled inside scinterm_notcurses_static — do not link separately.
set(li_link_libs lua lpeg lfs regex)

# Li Moon Notcurses platform.
# n_limoon.c uses GCC nested functions (progress_dialog) and notcurses.h
# needs _GNU_SOURCE for wcwidth/wcswidth. Override pedantic for this file only.
add_library(limoon_notcurses OBJECT src/n_limoon.c)
target_include_directories(limoon_notcurses PRIVATE
	${SCINTERM_NC_DIR}/include
	${SCINTERM_NC_DIR}/scintilla/include
	${NOTCURSES_INCLUDE_DIRS})
target_compile_definitions(limoon_notcurses PRIVATE _GNU_SOURCE)
target_compile_options(limoon_notcurses PRIVATE
	-Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers
	$<$<BOOL:${PROFILE}>:-pg --coverage>
	$<$<BOOL:${LIMOON_HOME}>:-DLIMOON_HOME="${LIMOON_HOME}">)
target_link_libraries(limoon_notcurses PRIVATE scinterm_notcurses_static lua)

# Li Moon executable.
add_executable(li ${li_src})
target_include_directories(li PRIVATE
	${SCINTERM_NC_DIR}/scintilla/include)
target_compile_options(li PRIVATE ${li_compile_opts})
target_link_options(li PRIVATE ${li_link_opts})
target_link_libraries(li PRIVATE ${li_link_libs} limoon_notcurses)

# Version information.
file(STRINGS core/init.lua version_line REGEX "^_RELEASE")
string(REGEX MATCH "[1-9][^']+" version ${version_line})
if(NOT NIGHTLY)
	string(REPLACE " " "_" version ${version})
else()
	set(version nightly)
endif()
if(NOT (WIN32 OR APPLE))
	string(APPEND version ".linux")
elseif(WIN32)
	string(APPEND version ".win")
elseif(APPLE)
	string(APPEND version ".macOS")
endif()

# Install/release.
function(install_data dir)
	install(FILES init.lua LICENSE DESTINATION ${dir})
	install(DIRECTORY core docs ${scintillua_SOURCE_DIR}/lexers test themes DESTINATION ${dir})
	install(DIRECTORY modules DESTINATION ${dir} PATTERN ".git" EXCLUDE PATTERN "build" EXCLUDE)
endfunction()
if(NOT (WIN32 OR APPLE))
	include(GNUInstallDirs)
	set(li_bin_dir ${CMAKE_INSTALL_FULL_BINDIR})
	install(CODE "file(MAKE_DIRECTORY ${li_bin_dir})")
	set(li_data_dir ${CMAKE_INSTALL_FULL_DATADIR}/limoon)
	file(RELATIVE_PATH li_bin_to_data_dir ${CMAKE_INSTALL_FULL_BINDIR} ${li_data_dir})
	if(DEFINED LIMOON_HOME)
		install(TARGETS li DESTINATION ${li_bin_dir})
	else()
		install(TARGETS li DESTINATION ${li_data_dir})
		install(CODE "file(CREATE_LINK ${li_bin_to_data_dir}/li
			${li_bin_dir}/li SYMBOLIC)")
	endif()
	install_data(${li_data_dir})
	install(FILES core/images/limoon.svg
		DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
	set(limoon_tgz limoon_${version}.tgz)
	if(CMAKE_SYSTEM_PROCESSOR MATCHES "^aarch")
		set(limoon_tgz limoon_${version}.arm.tgz)
	endif()
	# Note: can use file(ARCHIVE_CREATE ... WORKING_DIRECTORY) in CMake 3.31.
	add_custom_target(archive
		COMMAND ${CMAKE_COMMAND} -E tar czf ${CMAKE_BINARY_DIR}/${limoon_tgz} limoon
		WORKING_DIRECTORY ${CMAKE_INSTALL_FULL_DATADIR})
endif()

# Generate HTML.
option(GENERATE_HTML "Generate HTML documentation in docs/" OFF)
if(GENERATE_HTML)
	find_program(LUA lua REQUIRED)
	find_program(BUNDLE bundle REQUIRED)
	add_custom_target(html COMMAND ./gen_docs.sh WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/scripts)
endif()
