# Shared setup and helpers for the shell labs. Each lab.sh sources this file:
#   source "$(dirname "$0")/../lab_common.sh" "$@"
# It sets $src (the lab's source directory), creates and enters the work
# directory ($1, or a new temporary directory), and defines the helpers below.

src=$(cd "$(dirname "$0")" && pwd)
work=${1:-$(mktemp -d)}
mkdir -p "$work" && cd "$work"
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
