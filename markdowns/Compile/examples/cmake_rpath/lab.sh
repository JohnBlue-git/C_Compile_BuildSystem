#!/usr/bin/env bash
# CMake RPATH lab: configure, build, and install a small target-based project.
#
# Usage: bash lab.sh [work-dir]     (default: a new temporary directory)
set -euo pipefail

source "$(dirname "$0")/../lab_common.sh" "$@"

runpath() { printf '$ readelf -d %s | grep RUNPATH\n' "$1"; readelf -d "$1" | grep RUNPATH | tee out.txt; }

step "1. Configure and build; the verbose log shows the real g++ commands"
run cmake -S "$src" -B build -G Ninja -DCMAKE_BUILD_TYPE=Release > /dev/null
printf '$ cmake --build build --verbose | grep -o "c++ .*"\n'
cmake --build build --verbose | grep -o 'c++ .*' | sed "s|$src|<src>|g; s|$work|<work>|g"

step "2. Build tree: CMake writes the build directory into RUNPATH"
runpath build/app
expect out.txt "Library runpath: \[$work/build"
run ./build/app

step "3. Install: CMake replaces it with INSTALL_RPATH"
run cmake --install build --prefix stage
runpath stage/bin/app
expect out.txt 'Library runpath: \[\$ORIGIN/../lib\]'
run ./stage/bin/app

printf '\nLab passed.\n'
