#!/usr/bin/env bash
# Build pipeline lab: preprocess, compile, assemble, link, and inspect symbols.
#
# Usage: bash lab.sh [work-dir]     (default: a new temporary directory)
# Every command is printed before it runs. All output files go to work-dir.
set -euo pipefail

src=$(cd "$(dirname "$0")" && pwd)
work=${1:-$(mktemp -d)}
mkdir -p "$work" && cd "$work"
cp "$src"/*.c "$src"/*.cpp .
echo "work dir: $work"

step() { printf '\n=== %s ===\n' "$*"; }
run()  { printf '$ %s\n' "$*"; "$@"; }
# Run a command that must fail; show its error output.
fails() {
    printf '$ %s    # expected to fail\n' "$*"
    if "$@" > out.txt 2>&1; then cat out.txt; echo "LAB ERROR: command succeeded"; exit 1; fi
    cat out.txt
}
# Assert that a file contains a pattern (makes the lab a real test).
expect() { grep -q -- "$2" "$1" || { echo "LAB ERROR: '$2' not found in $1"; exit 1; }; }

step "1. One stage at a time (C)"
run gcc -E main.c -o main.i
printf '$ grep -c "" main.i      # line count: <stdio.h> was pasted in\n'; grep -c "" main.i
printf '$ tail -n 4 main.i      # TWICE() is gone: macros are expanded\n'; tail -n 4 main.i | tee tail.txt
expect tail.txt '((add(2, 3)) \* 2)'
run gcc -S main.i -o main.s
printf '$ grep call main.s\n'; grep call main.s | tee call.txt
expect call.txt 'call	add'
run gcc -c main.s -o main.o
run nm main.o | tee nm.txt
expect nm.txt ' U add$'
run gcc -c add.c -o add.o
run nm add.o | tee nm.txt
expect nm.txt ' T add$'
run gcc main.o add.o -o app
run ./app | tee out.txt
expect out.txt '^10$'

step "2. Name mangling: the same function as C and as C++"
run gcc -c add.c -o add_c.o
run g++ -c add.cpp -o add_cpp.o
run g++ -c overload.cpp -o overload.o
run g++ -c add_c_linkage.cpp -o add_c_linkage.o
run nm add_c.o
run nm add_cpp.o | tee nm.txt
expect nm.txt ' T _Z3addii$'
run nm overload.o
run nm -C overload.o
run c++filt _Z3adddd
run nm add_c_linkage.o | tee nm.txt
expect nm.txt ' T add$'

step "3. A C caller needs the C symbol name"
fails gcc main.o add_cpp.o -o app_mangled
expect out.txt "undefined reference to \`add'"
run gcc main.o add_c_linkage.o -o app_extern_c
run ./app_extern_c

step "4. gcc vs g++ as the link driver"
run g++ -c hello.cpp -o hello.o
fails gcc hello.o -o app_gcc
expect out.txt "undefined reference to \`std::cout'"
run gcc hello.o -o app_gcc -lstdc++
run ./app_gcc
run g++ hello.o -o app_gxx
run ./app_gxx

printf '\nLab passed.\n'
