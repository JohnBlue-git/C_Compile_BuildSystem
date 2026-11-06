#!/usr/bin/env bash
# Linkage lab: definitions in headers, tentative definitions, const, static vs inline.
#
# Usage: bash lab.sh [work-dir]     (default: a new temporary directory)
set -euo pipefail

source "$(dirname "$0")/../lab_common.sh" "$@"
cp "$src"/*.h "$src"/*.c "$src"/*.cpp .

step "1. A variable defined in a header -> multiple definition"
run gcc -c use_a.c -o use_a.o
run gcc -c use_b.c -o use_b.o
run nm use_a.o
fails gcc use_a.o use_b.o -o app_counter
expect out.txt "multiple definition of \`counter'"

step "2. Tentative definitions: -fno-common (default since GCC 10) vs -fcommon"
run gcc -c tentative_a.c -o tentative_a.o
run gcc -c tentative_b.c -o tentative_b.o
run nm tentative_a.o
fails gcc tentative_a.o tentative_b.o -o app_tentative
expect out.txt "multiple definition of \`shared_flag'"
run gcc -fcommon -c tentative_a.c -o tentative_a.o
run gcc -fcommon -c tentative_b.c -o tentative_b.o
run nm tentative_a.o | tee nm.txt
expect nm.txt ' C shared_flag$'
run gcc tentative_a.o tentative_b.o -o app_tentative

step "3. const int MAX = 100; in a header: C vs C++"
run gcc -c config_a.c -o config_a.o
run nm config_a.o | tee nm.txt
expect nm.txt ' R MAX$'
run gcc -c config_b.c -o config_b.o
fails gcc config_a.o config_b.o -o app_config_c
expect out.txt "multiple definition of \`MAX'"
run g++ -x c++ -c config_a.c -o config_a_cpp.o
run nm config_a_cpp.o | tee nm.txt
expect nm.txt ' r _ZL3MAX$'
run g++ config_main.cpp config_a_cpp.o -o app_config_cpp
run ./app_config_cpp

step "4. static vs inline in a C++ header"
run g++ -c helpers_a.cpp -o helpers_a.o
run nm -C helpers_a.o | tee nm.txt
expect nm.txt ' W next_inline()$'
expect nm.txt ' t next_static()$'
run g++ helpers_main.cpp helpers_a.o -o app_helpers
run ./app_helpers | tee out.txt
expect out.txt 'static: 1, inline: 2'

printf '\nLab passed.\n'
