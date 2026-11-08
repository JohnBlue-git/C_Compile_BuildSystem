# Meson and Ninja

Meson is a build system that configures project targets and normally uses Ninja to execute the generated build graph. This repository uses Ninja as its Meson backend; `meson compile` selects the configured backend. Use the same Docker image as CMake with `make meson-build`, `make meson-test`, and `make meson-run`.

## Quick start

On the host, only Docker and GNU Make are required:

```sh
make image
make meson-build
make meson-test
make meson-run EX=cxx_move_semantics
make meson-test PRESET=asan
```

Inside `make shell`, the direct commands are:

```sh
meson setup build/meson/debug --buildtype debug
meson compile -C build/meson/debug
meson test -C build/meson/debug --print-errorlogs
meson test -C build/meson/debug --suite memory
```

Each build directory is configured once for one compiler and build type. To change its options, run `meson setup --reconfigure ...`; use a different build directory to change compilers. Meson stores compiler selection in the build directory, just as a CMake build tree does.

| `PRESET` | Meson options | Compiler |
|---|---|---|
| `debug` | `--buildtype debug` | gcc/g++ |
| `release` | `--buildtype release -Doptimization=2` | gcc/g++ |
| `asan` | `--buildtype debug -Db_sanitize=address,undefined` | gcc/g++ |
| `clang` | `--buildtype debug` | clang/clang++ |

Build output goes to `build/meson/<preset>/`. The Makefile sets `CC` and `CXX` for the `clang` preset when that build directory is first configured.

## Meson compared with CMake

| Concern | CMake in this repository | Meson in this repository |
|---|---|---|
| Project entry | `CMakeLists.txt` | `meson.build` |
| Configure | `cmake --preset debug` | `meson setup build/meson/debug --buildtype debug` |
| Compile | `cmake --build --preset debug` | `meson compile -C build/meson/debug` |
| Run tests | `ctest --preset debug` | `meson test -C build/meson/debug --print-errorlogs` |
| Select a topic | CTest label, for example `-L cxx` | Test suite, for example `--suite cxx` |
| Persistent user options | Cache variables in `CMakeCache.txt` | Built-in and project options stored in the build directory |
| Dependency lookup | `find_package()` | `dependency()` |
| Source lists | Explicit target source lists | Explicit target source lists |

Meson intentionally keeps source lists explicit. When adding an example, add its source and test metadata to the `example_specs` list in [meson.build](../../meson.build). Add a playground program as an `executable()` target in [playground/meson.build](../../playground/meson.build).

## Targets and tests

The root [meson.build](../../meson.build) mirrors the repository's CMake examples: C17 and C++20, warning level 3, optional JSON dependencies, pthread examples, shared plugins for the `dlopen` demo, and test suites named `c`, `cxx`, `compile`, and `memory`.

A Meson executable and its test are separate declarations:

```meson
app = executable('app', 'app.cpp', dependencies: [threads_dep])
test('app', app, suite: 'cxx', timeout: 30)
```

Tests may receive built targets as arguments, which is how the `dlopen` test receives its plugin paths. The four Compile shell labs remain ordinary Meson tests with a longer timeout; deliberate memory-bug examples are built but not registered as tests.

`dependency('nlohmann_json', required: false)` and `dependency('boost', modules: ['json'], required: false)` let the JSON examples be skipped when those development packages are absent, matching the CMake build.

## Docker toolchain

The repository installs Meson and Ninja from the same fixed Ubuntu archive snapshot as GCC, Clang, CMake, and the debugging tools. This keeps both build systems in one reproducible environment. The Meson project also publishes [`mesonbuild/ubuntu-rolling`](https://hub.docker.com/r/mesonbuild/ubuntu-rolling) on Docker Hub; it is a useful ready-made Meson image, but its rolling base is not pinned to this repository's Ubuntu snapshot.

This setup follows the container-first approach used in [HowToSDBusPlus](https://github.com/JohnBlue-git/HowToSDBusPlus), while avoiding that project's extra SDBusPlus/systemd dependencies because these examples do not need them.

## References

- [Meson: Getting Meson](https://mesonbuild.com/Getting-meson.html)
- [Meson: Quick guide](https://mesonbuild.com/Quick-guide.html)
- [HowToSDBusPlus](https://github.com/JohnBlue-git/HowToSDBusPlus)
