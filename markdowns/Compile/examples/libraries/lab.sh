#!/usr/bin/env bash
# Library lab: static vs shared libraries, NEEDED entries, and the load stage.
#
# Usage: bash lab.sh [work-dir]     (default: a new temporary directory)
set -euo pipefail

source "$(dirname "$0")/../lab_common.sh" "$@"
cp "$src"/*.c "$src"/*.cpp .

needed() { printf '$ readelf -d %s | grep NEEDED\n' "$1"; readelf -d "$1" | grep NEEDED || echo "(no NEEDED entries)"; }

step "1. Static library: the code is copied into the executable"
run gcc -c add.c -o add.o
run ar rcs libadd.a add.o
run ar t libadd.a
run gcc main.c -L. -ladd -o app_static
run ./app_static
needed app_static | tee out.txt
if grep -q libadd out.txt; then echo "LAB ERROR: app_static depends on libadd"; exit 1; fi

step "2. Shared libraries: NEEDED lists only what is really used"
run gcc -fPIC -shared add.c -o libadd_c.so
run g++ -fPIC -shared add.cpp -o libadd_cpp.so
run g++ -fPIC -shared greet.cpp -o libgreet.so
needed libadd_c.so
needed libadd_cpp.so | tee out.txt
if grep -q libstdc++ out.txt; then echo "LAB ERROR: unexpected libstdc++"; exit 1; fi
needed libgreet.so | tee out.txt
expect out.txt 'libstdc++.so.6'
printf '$ ldd libgreet.so | sed "s/ (0x.*//"\n'; ldd libgreet.so | sed 's/ (0x.*//'

step "3. Link succeeds, load fails: the loader does not search the current directory"
run gcc main.c -L. -ladd_c -o app_shared
needed app_shared
fails ./app_shared
expect out.txt 'libadd_c.so: cannot open shared object file'
printf '$ LD_LIBRARY_PATH=. ./app_shared\n'; LD_LIBRARY_PATH=. ./app_shared

step "4. Record the search path in the executable (RUNPATH)"
printf '$ gcc main.c -L. -ladd_c -Wl,-rpath,'"'"'$ORIGIN'"'"' -o app_rpath\n'
gcc main.c -L. -ladd_c -Wl,-rpath,'$ORIGIN' -o app_rpath
printf '$ readelf -d app_rpath | grep -E "RPATH|RUNPATH"\n'; readelf -d app_rpath | grep -E 'RPATH|RUNPATH' | tee out.txt
expect out.txt 'Library runpath: \[\$ORIGIN\]'
run ./app_rpath

printf '\nLab passed.\n'
