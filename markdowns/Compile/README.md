# Compilation and Linking: Interview Notes

This directory covers the build layer that C and C++ share: the preprocessor, translation units, object files, the linker, linkage, the ODR, the ABI, static and shared libraries, the dynamic loader, and CMake. It brings the compile/link material from the C and CMake sections into one path.

| Note | Contents |
|---|---|
| [00_compile_and_linkage.md](00_compile_and_linkage.md) | Build pipeline, name mangling and `extern "C"`, gcc vs g++ linking, shared libraries and `dlopen`, C linkage, C++ ODR, ABI boundary, debugging tools |
| [01_gcc_gxx_cmake_basics.md](01_gcc_gxx_cmake_basics.md) | gcc/g++ by hand vs CMake, legacy vs target-based CMake, RPATH in the build tree and after install |
| [02_modern_cmake.md](02_modern_cmake.md) | Target-based CMake, dependencies, sanitizers, presets, packaging, common traps, and this repo's build as a worked example |

## Examples

Sources are in [examples/](examples/). The shell labs call `gcc`, `g++`, `nm`, `readelf`, and `cmake` directly and print each command, so you see the raw toolchain. They write only to a temporary directory (or to the directory given as the first argument). Every lab checks its own results, so it is also a CTest test.

| Target / test | What it shows | How to run |
|---|---|---|
| `compile_pipeline_lab` | `gcc -E/-S/-c`/link one stage at a time; `nm` of C vs C++ symbols; overloads and `c++filt`; `extern "C"`; `gcc` vs `g++` as link driver | `make shell`, then `bash markdowns/Compile/examples/pipeline/lab.sh` |
| `compile_linkage_lab` | Header definitions and `multiple definition`; tentative definitions with `-fno-common`/`-fcommon`; `const` in C vs C++; `static` vs `inline` in a header | `bash markdowns/Compile/examples/linkage/lab.sh` (in `make shell`) |
| `compile_libraries_lab` | Static library with `ar`; `NEEDED` entries and `--as-needed`; "cannot open shared object file"; `LD_LIBRARY_PATH` vs `-Wl,-rpath,'$ORIGIN'` | `bash markdowns/Compile/examples/libraries/lab.sh` (in `make shell`) |
| `compile_cmake_rpath_lab` | A target-based CMake project: the real g++ commands, build-tree `RUNPATH`, `INSTALL_RPATH` after `cmake --install` | `bash markdowns/Compile/examples/cmake_rpath/lab.sh` (in `make shell`) |
| `compile_mixed_c_cxx` | A C library header with the `#ifdef __cplusplus extern "C"` guard, called from C++ | `make run EX=compile_mixed_c_cxx` |
| `compile_dlopen_loader` | `dlopen`/`dlsym`: `"add"` is not found in a C++ plugin (`_Z3addii`), but is found with `extern "C"` | `make run EX=compile_dlopen_loader` |

Run all of them as tests: `make test`, or inside `make shell`: `ctest --preset debug -L compile`.

## This repository's build is an example too

The repo's own build is a small, target-based CMake project. [02_modern_cmake.md, section 7](02_modern_cmake.md#7-a-worked-example-this-repositorys-build) walks through it:

- [CMakeLists.txt](../../CMakeLists.txt): C17/C++20 defaults with extensions off, `enable_testing()`, one `add_subdirectory()` per topic.
- [cmake/Examples.cmake](../../cmake/Examples.cmake): the `example_warnings` `INTERFACE` target (`-Wall -Wextra -Wpedantic`) and the `add_example()` function, which creates a target and a labeled CTest test (`NO_TEST`, `ARGS`, `DATA`, `LIBS`).
- [CMakePresets.json](../../CMakePresets.json): the `debug`, `release`, `asan` (AddressSanitizer + UBSan), and `clang` presets, each with its own `build/<preset>` directory.
- [Makefile](../../Makefile): runs everything inside the Docker toolchain image (`make build|test|run|shell`, `PRESET=...`).

## Related notes

- [C notes](../C/README.md): types, pointers, memory, structs, and undefined behavior.
- [C++ notes](../CXX/README.md): RAII, smart pointers, concurrency, containers, OOP and design patterns.
- [Memory notes](../Memory/README.md): stack, heap, object lifetime, and sanitizer demos.
