# C and C++: Concepts, Examples, and Build Tools

Tutorials on C, C++, memory, and the build toolchain (gcc/g++, the linker,
CMake, Meson, and Ninja). Each topic connects its concepts to runnable examples,
and one Docker image provides a consistent toolchain for both build systems.

## Topics

| Directory | What it covers |
|---|---|
| [C/](markdowns/C/README.md) | Pointers and arrays, types and `sizeof`, callbacks, strings and copy APIs, qualifiers, preprocessor, signals, file I/O |
| [CXX/](markdowns/CXX/README.md) | RAII, smart pointers, concurrency, move semantics and RVO, lambdas, casts and exceptions, C++11–20, STL containers, OOP and design patterns, streams, JSON |
| [Compile/](markdowns/Compile/README.md) | Preprocess → compile → assemble → link → load, linkage, ODR, ABI, `extern "C"`, static and shared libraries, `dlopen`, gcc vs g++, Make, Autotools, CMake, Meson/Ninja |
| [Memory/](markdowns/Memory/README.md) | Stack, heap, and static storage, object lifetime, callback lifetime, sanitizer bug demos |
| [playground/](playground/README.md) | Scratch area: drop in a `.c`/`.cpp` file, then build and run it |

Suggested learning path:
**Compile → C → Memory → CXX** (core 00–05, then 06–10 as needed).

## Quick start

Requirements: Docker and GNU Make. You do not need a compiler, CMake, Meson, or Ninja on the host.

```bash
make image                            # build the toolchain image (once, about 2 minutes)
make build                            # configure and build all examples
make test                             # build all examples and run them as tests
make run EX=cxx_move_semantics        # build and run one example
make test PRESET=asan                 # all examples again, with AddressSanitizer + UBSan
make meson-build                      # build all examples with Meson/Ninja
make meson-test PRESET=asan            # Meson/Ninja tests with sanitizers
make meson-run EX=cxx_move_semantics    # build and run one Meson target
make shell                            # interactive shell with gcc, clang, cmake, meson, ninja, gdb, valgrind
```

Each example's target name is listed in the README of its topic directory.
The Compile shell labs (`compile_*_lab`) are tests, not build targets, so
`make run` cannot run them. Run them with `bash markdowns/Compile/examples/<lab>/lab.sh`,
`ctest --preset debug -R <name>`, or `meson test -C build/meson/debug <name>` inside
`make shell`.

Inside `make shell` you use plain CMake:

```bash
cmake --preset debug && cmake --build --preset debug
ctest --preset debug -L '^memory$'    # one topic: c, cxx, compile, memory (-L is a regex)
```

Or use Meson with Ninja in the same container:

```bash
meson setup build/meson/debug --buildtype debug
meson compile -C build/meson/debug
meson test -C build/meson/debug --print-errorlogs
```

`make meson-build`, `make meson-test`, and `make meson-run EX=<target>` are
Docker-backed shortcuts. They accept the same `PRESET=debug|release|asan|clang`
names as the CMake commands.

### Build presets

| Preset | Compiler | Use it for |
|---|---|---|
| `debug` (default) | gcc 13 | Normal work |
| `release` | gcc 13, `-O2` | Benchmarks (for example the CRTP benchmark), optimizer behavior |
| `asan` | gcc 13, `-fsanitize=address,undefined` | Finding memory bugs and UB, and the Memory bug demos |
| `clang` | clang 18 | Comparing diagnostics and behavior with a second compiler |

CMake presets build into `build/<preset>/`; Meson builds into
`build/meson/<preset>/`. Run `make clean` to delete all build output.

## Toolchain image

[docker/Dockerfile](docker/Dockerfile) is based on `ubuntu:24.04`, **pinned by
digest**, and apt installs from a fixed date of the Ubuntu archive
(`UBUNTU_SNAPSHOT`, served by snapshot.ubuntu.com). Every rebuild therefore
gets the same OS layer and the same package versions. Only `ca-certificates`
comes from the live archive, because apt needs it to reach the HTTPS snapshot
server.

| Tool | Version |
|---|---|
| gcc / g++ | 13.3 |
| clang / clang++ | 18.1 |
| CMake / Meson / Ninja | 3.28 / 1.3.2 / 1.11 |
| gdb / valgrind | 15.1 / 3.22 |
| Boost.JSON / nlohmann-json | 1.83 / 3.11.3 |
| Other tools | lld, strace, binutils (`nm`, `objdump`, `readelf`), `file`, `less` |

Why this image and not the official `gcc:<version>` image: the `gcc` image
doesn't include CMake, clang, gdb, or valgrind, and its tags are rebuilt often.
Ubuntu LTS gives two compilers and all the debugging tools from one stable
package set.

The Meson project publishes a [`mesonbuild/ubuntu-rolling` container](https://hub.docker.com/r/mesonbuild/ubuntu-rolling),
but its rolling base is less suitable for reproducible builds. This repository
installs Meson from the same pinned Ubuntu snapshot as the rest of the toolchain.

The container runs as your host user, so build output in `build/` belongs to
you and not to root. It also has `SYS_PTRACE` and runs with `--security-opt seccomp=unconfined`,
so gdb can turn off address randomization and the sanitizers work.

## Repository conventions

- Each topic has `NN_topic.md` notes and an `examples/` directory.
- Examples are registered with `add_example(<name> SOURCES ...)` from
  [cmake/Examples.cmake](cmake/Examples.cmake). Each one is also a CTest test,
  labeled by topic. Target names start with the topic prefix: `c_`, `cxx_`,
  `compile_`, `memory_`.
- Warnings are on for every example (`-Wall -Wextra -Wpedantic`) through the
  `example_warnings` interface target. The examples compile without warnings.
- Standards: C17 and C++20, with compiler extensions off.
- The repository's own build files are worked examples of target-based CMake
  and Meson. Read [Compile/02_modern_cmake.md](markdowns/Compile/02_modern_cmake.md)
  and [Compile/03_meson.md](markdowns/Compile/03_meson.md).
