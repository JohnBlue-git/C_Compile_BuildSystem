# Modern CMake (Build Systems)

> Interviewers use CMake questions to find out whether you have really worked on a production build. The core topic is the difference between **Modern (target-based) CMake** and **old (variable-based) CMake**. For a stage-by-stage comparison (include/compile/link/load/install), read sections 3 and 4 of [gcc/g++ and CMake basics](01_gcc_gxx_cmake_basics.md) first. This note assumes you know the core idea, "target-based CMake replaces directory-scoped variables with per-target properties", and covers the details and traps of production projects.

This repository's own build is a small, working example of target-based CMake. Section 7 walks through it, and the sections below point to it where it applies.

## 1. The core idea of Modern CMake: targets

Think of a target (`add_executable`, `add_library`) as a build unit with properties. Its include paths, macros, libraries, and compile flags belong to the target, together with the decision whether each one passes on to the targets that link it. They are not spread across global variables in `CMakeLists.txt`. If an answer uses many global commands such as `include_directories()`, `link_directories()`, or a global `CMAKE_CXX_FLAGS`, the dependency boundaries are usually unclear. The [basics note](01_gcc_gxx_cmake_basics.md#33-the-problem-with-non-target-based-cmake-directory-scope-leaks) shows how those global commands make unrelated targets inherit include paths and libraries they do not need. That is the core problem Modern CMake solves.

### `PUBLIC` / `PRIVATE` / `INTERFACE`

| Visibility | Does this target need it to build? | Do targets that link this target need it? |
|---|---|---|
| `PRIVATE` | Yes | No |
| `INTERFACE` | No (for example, a header-only library) | Yes |
| `PUBLIC` | Yes | Yes |

```cmake
add_library(my_lib STATIC my_lib.cpp)
add_executable(app main.cpp)
target_link_libraries(my_lib PUBLIC api_types PRIVATE crypto)
target_link_libraries(app PRIVATE my_lib)
```

`PUBLIC`/`PRIVATE`/`INTERFACE` control CMake's dependency propagation. They have nothing to do with C/C++ `static`, `extern`, or linkage. They also act only on the **Compile and Link stages**: which include paths a target sees, and which libraries appear on its link line. They do not affect the **Load stage**. Whether `app` can find `my_lib`'s `.so` at run time (if it is `SHARED`) depends on `RUNPATH`, not on these keywords. Section 6 covers this trap for `SHARED` libraries.

(A detail about `STATIC` libraries: even when `my_lib` links `crypto` as `PRIVATE`, CMake still adds `crypto` to `app`'s link line. A static archive cannot carry its own dependencies, so the final link must include them. `PRIVATE` still hides `crypto`'s include paths and compile definitions from `app`.)

An `INTERFACE` library does not have to be header-only code. It can be a bundle of usage requirements. This repo's [cmake/Examples.cmake](../../cmake/Examples.cmake) uses one for warnings:

```cmake
add_library(example_warnings INTERFACE)
target_compile_options(example_warnings INTERFACE
    $<$<OR:$<COMPILE_LANG_AND_ID:C,GNU,Clang>,$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>>:-Wall -Wextra -Wpedantic>)
```

Every example then calls `target_link_libraries(<name> PRIVATE example_warnings)`. It gets the flags, nothing is built for `example_warnings`, and the flags do not leak to targets that do not link it.

### ALIAS targets: one spelling for in-tree and external dependencies

The next step of the target idea is an `ALIAS` with a namespace. The caller does not need to know whether the dependency is a target in this project, one from `FetchContent`, or an imported target from `find_package`:

```cmake
add_library(my_lib STATIC my_lib.cpp)
add_library(MyProject::my_lib ALIAS my_lib)

# Callers always write MyProject::my_lib, wherever it comes from
target_link_libraries(app PRIVATE MyProject::my_lib)
```

The `MyLibs::some_lib` imported target in section 6 uses the same convention. A name with `::` is a common Modern CMake convention. It sets namespaced targets apart from plain ones, and it avoids name collisions. It also catches typos: CMake treats a name with `::` in `target_link_libraries` as a target and stops with an error if the target does not exist. A plain name that is not a target is passed to the linker as `-lname`.

## 2. Third-party dependencies

- `FetchContent`: downloads the source at configure time and builds it with your project. Good for dependencies that must be built together with the project. The targets from `FetchContent_MakeAvailable` are ordinary in-tree targets, so the `PUBLIC`/`PRIVATE`/`ALIAS` rules above apply to them.
- `find_package()`: finds a package from the system or from a package manager (vcpkg, Conan, the distribution). It looks for a package config file or a CMake Find module. If it finds one, it usually provides an imported target whose `INTERFACE_INCLUDE_DIRECTORIES` are already set, for example `Boost::boost`.

```cmake
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(googletest)
target_link_libraries(my_tests PRIVATE gtest_main)
```

Pin `GIT_TAG` to a fixed tag or commit hash, not a branch name. Otherwise the same `CMakeLists.txt` can fetch different sources at different times, and the build is no longer reproducible.

```cmake
find_package(Boost REQUIRED)
target_link_libraries(app PRIVATE Boost::boost)
```

## 3. Compile options and sanitizers

```cmake
# The C++ standard for one target
target_compile_features(my_target PRIVATE cxx_std_20)
target_compile_options(my_target PRIVATE -Wall -Wextra -fsanitize=address)
target_link_options(my_target PRIVATE -fsanitize=address)
```

Options attached to a target do not affect unrelated executables or libraries. A sanitizer needs both the compile option and the link option (the runtime library comes in at link time).

`target_compile_features(... cxx_std_20)` sets the minimum standard, but CMake still uses the GNU dialect (`-std=gnu++20`) unless the `CXX_EXTENSIONS` property is `OFF`. The `compile_cmake_rpath_lab` output in the [basics note](01_gcc_gxx_cmake_basics.md#33-the-problem-with-non-target-based-cmake-directory-scope-leaks) shows this. This repo's root `CMakeLists.txt` sets `CMAKE_CXX_STANDARD 20`, `CMAKE_CXX_STANDARD_REQUIRED ON`, and `CMAKE_CXX_EXTENSIONS OFF` (plus the C17 equivalents) once, as the default for every target, so all examples compile with plain `-std=c++20` / `-std=c17`.

If an option depends on the build type, use a generator expression instead of a global `if(CMAKE_BUILD_TYPE STREQUAL "Debug")` branch:

```cmake
target_compile_options(my_target PRIVATE
    $<$<CONFIG:Debug>:-g -O0>
    $<$<CONFIG:Release>:-O2>
)
```

(CMake's defaults for GCC already give Debug `-g` and Release `-O3 -DNDEBUG`. Use this for flags beyond those. A later `-O2` overrides the default `-O3`.) A generator expression `$<condition:value>` is the same mechanism as `$<BUILD_INTERFACE:...>`/`$<INSTALL_INTERFACE:...>` in section 5. It expands at generate time, so it also works with multi-config generators (Visual Studio, Ninja Multi-Config), where `CMAKE_BUILD_TYPE` is not used at configure time.

### Sanitizers per target or per build?

Per-target options are the default rule. Sanitizers are a common exception. ASan must be in the executable's link step, and it is most useful when **all** code, including shared libraries, is instrumented. So it is often a property of the whole *build*, not of one target. This repo's `asan` preset in [CMakePresets.json](../../CMakePresets.json) does exactly that. It sets `CMAKE_C_FLAGS`/`CMAKE_CXX_FLAGS` to `-fsanitize=address,undefined -fno-omit-frame-pointer` and adds the same flag to the executable and shared-library linker flags. The `CMakeLists.txt` files do not change at all. The [Memory examples](../Memory/README.md) use this preset.

## 4. Out-of-source builds

Run CMake in a separate `build/` directory. The source tree stays clean, and Debug and Release builds can exist at the same time:

```text
project/
├── src/
├── include/
├── CMakeLists.txt
├── build-debug/
└── build-release/
```

```sh
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug
```

### Presets: name the build configurations

`CMakePresets.json` (CMake 3.19+) puts these command lines into a file under version control. This repo's [CMakePresets.json](../../CMakePresets.json) has a hidden `base` preset (Ninja, `binaryDir` = `build/<preset name>`) and four presets that inherit from it:

| Preset | What it sets |
|---|---|
| `debug` | `CMAKE_BUILD_TYPE=Debug`, `gcc`/`g++` |
| `release` | Inherits `debug`, changes `CMAKE_BUILD_TYPE` to `Release` |
| `asan` | Inherits `debug`, adds `-fsanitize=address,undefined` to the compile and link flags |
| `clang` | `CMAKE_BUILD_TYPE=Debug`, `clang`/`clang++` |

```sh
cmake --preset asan              # configure into build/asan
cmake --build --preset asan
ctest --preset asan              # test presets set outputOnFailure
```

Each preset gets its own build directory, which is the out-of-source idea above.

## 5. `target_include_directories`: build vs install

```cmake
target_include_directories(mylib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
```

This separates the include path used while building this project from the path that users of the installed library should use. To make the library a package that other projects can find with `find_package()`, also export the target with `install(EXPORT ...)`:

```cmake
include(GNUInstallDirs)

install(TARGETS mylib
    EXPORT mylibTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT mylibTargets
    FILE mylibTargets.cmake
    NAMESPACE MyLib::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mylib
)
```

`GNUInstallDirs` provides the standard install directories (`lib`/`lib64`/`lib/<multiarch>`, `bin`, `include`) for the platform. `install(EXPORT ...)` writes the target, with its `PUBLIC`/`INTERFACE` properties, to a `.cmake` file. One more file is required: `find_package(mylib)` looks for `mylibConfig.cmake` (or `mylib-config.cmake`), not `mylibTargets.cmake`. The usual way is a small `mylibConfig.cmake` that does `include("${CMAKE_CURRENT_LIST_DIR}/mylibTargets.cmake")` (often generated with `configure_package_config_file()` from `CMakePackageConfigHelpers`, plus a version file from `write_basic_package_version_file()`), installed to the same directory. After that, a downstream project gets the imported target `MyLib::mylib`, and it behaves like the `ALIAS` target in section 1.

## 6. Common traps

### Do not hide the source list behind `file(GLOB ...)`

```cmake
file(GLOB SOURCES src/*.cpp)
```

CMake evaluates the glob at configure time. If you add a source file and do not configure again, the build system does not see it. List sources explicitly, or configure again after changes. `file(GLOB ... CONFIGURE_DEPENDS ...)` re-checks the glob at every build, but CMake's documentation still recommends explicit lists, and the check does not work with every generator.

### `STATIC`, `SHARED`, `INTERFACE`

| Type | Description |
|---|---|
| `STATIC` | Archive (`.a`/`.lib`); its objects are linked into the executable |
| `SHARED` | Dynamic library (`.so`/`.dll`); loaded at run time |
| `INTERFACE` | Produces no binary; carries only usage requirements (header-only libraries, flag bundles such as `example_warnings`) |

`SHARED` is the only type that spans both Link and Load, and it is the easiest to configure incompletely. `target_link_libraries` only makes sure that symbols resolve at **Link**. It does not make sure the executable can find the `.so` at **Load**. As shown in the [basics note](01_gcc_gxx_cmake_basics.md), CMake writes a build-tree `RUNPATH` by default, so programs run from the build tree work. After `cmake --install`, CMake removes that path, and you must add your own:

```cmake
add_library(my_shared SHARED my_shared.cpp)

set_target_properties(app PROPERTIES
    INSTALL_RPATH "$ORIGIN/../lib"          # only this target's installed binary
    BUILD_WITH_INSTALL_RPATH FALSE          # the default: build tree uses the build RUNPATH
)
```

> Run it: `bash markdowns/Compile/examples/cmake_rpath/lab.sh` inside `make shell` (CTest: `compile_cmake_rpath_lab`). It prints the `RUNPATH` before and after install.

The basics note sets a global `CMAKE_INSTALL_RPATH`, which initializes every target created after it. Here, `set_target_properties(... INSTALL_RPATH ...)` sets one target. This is the target-based idea applied to Load: even RPATH, which people often set globally, belongs on a target property in Modern CMake.

`dlopen`ed plugins are a special case. `target_link_libraries` is not used for them at all, so CMake does not know about the run-time dependency. This repo's `compile_dlopen_loader` example adds the build-order dependency itself with `add_dependencies(compile_dlopen_loader compile_plugin_mangled compile_plugin_extern_c)`, and it passes the plugin paths to the test with `$<TARGET_FILE:...>` (see [Compile/examples/CMakeLists.txt](examples/CMakeLists.txt)).

### Use imported targets instead of global link paths

```cmake
add_library(MyLibs::some_lib STATIC IMPORTED)
set_target_properties(MyLibs::some_lib PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/libs/libsome_lib.a"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
add_executable(my_app main.cpp)
target_compile_features(my_app PRIVATE cxx_std_17)
target_link_libraries(my_app PRIVATE MyLibs::some_lib)
```

Compared with `-L./libs -lsome_lib`, an imported target ties the library path and its include requirements to one target, so the linker is less likely to pick up an old system copy by mistake. If the project keeps prebuilt Debug and Release versions, set `IMPORTED_LOCATION` per configuration, so a Debug executable does not link the Release `.a`:

```cmake
set_target_properties(MyLibs::some_lib PROPERTIES
    IMPORTED_LOCATION_DEBUG   "${CMAKE_CURRENT_SOURCE_DIR}/libs/debug/libsome_lib.a"
    IMPORTED_LOCATION_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/libs/release/libsome_lib.a"
)
```

## 7. A worked example: this repository's build

The repo's build files are short. Read them as a small target-based project:

| File | What it shows |
|---|---|
| [CMakeLists.txt](../../CMakeLists.txt) | `project(... LANGUAGES C CXX)`; the C17/C++20 defaults with extensions off; `CMAKE_EXPORT_COMPILE_COMMANDS ON` (writes `compile_commands.json` for clangd and other tools); `enable_testing()`; one `add_subdirectory()` per topic |
| [cmake/Examples.cmake](../../cmake/Examples.cmake) | The `example_warnings` `INTERFACE` target (section 1), and the `add_example()` function |
| [CMakePresets.json](../../CMakePresets.json) | The `debug`/`release`/`asan`/`clang` presets (section 4) |
| [Makefile](../../Makefile) | A thin wrapper that runs every step inside the Docker toolchain image |

`add_example()` is a function that bundles a target and a test into one call:

```cmake
function(add_example name)
    cmake_parse_arguments(PARSE_ARGV 1 EX "NO_TEST" "" "SOURCES;LIBS;DATA;ARGS")

    add_executable(${name} ${EX_SOURCES})
    target_link_libraries(${name} PRIVATE example_warnings ${EX_LIBS})
    # ... copy DATA files, then:
    if(NOT EX_NO_TEST)
        add_test(NAME ${name} COMMAND ${name} ${EX_ARGS}
                 WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
        set_tests_properties(${name} PROPERTIES TIMEOUT 30 LABELS "${EXAMPLE_LABEL}")
    endif()
endfunction()
```

Points worth noting:

- **Everything is per target.** Warnings come from `example_warnings` through `target_link_libraries`, and extra libraries come through `LIBS` (for example, `LIBS ${CMAKE_DL_LIBS}` for the dlopen loader). Nothing uses `include_directories()` or `link_libraries()`.
- **`cmake_parse_arguments`** gives a CMake function keyword arguments (`NO_TEST`, `SOURCES ...`), like the built-in commands.
- **CTest labels.** Each `examples/CMakeLists.txt` sets `EXAMPLE_LABEL` (`c`, `cxx`, `compile`, `memory`) before it calls `add_example()`, and every test gets that label. You can then run one topic: `ctest --preset debug -L compile`. Tests that are not executables use the same labels. The shell labs in [Compile/examples/CMakeLists.txt](examples/CMakeLists.txt) are registered with `add_test(NAME compile_pipeline_lab COMMAND bash .../lab.sh ...)` and `set_tests_properties(... LABELS compile TIMEOUT 60)`.
- **`NO_TEST`.** Deliberate bug demos (such as `memory_bug_use_after_free`) are built with `NO_TEST`, so `ctest` stays green. You run them by hand with the `asan` preset.
- **The `Makefile` adds no build logic.** `make test PRESET=asan` runs `cmake --preset asan && cmake --build --preset asan && ctest --preset asan` in the container. `make run EX=<target>` builds one target and runs it from its build directory.

## Further reading

- [gcc/g++ and CMake basics](01_gcc_gxx_cmake_basics.md): target-based vs non-target-based CMake side by side, and how Link and Load differ in both styles.
- [C/C++ Compilation and Linking](00_compile_and_linkage.md): translation units, linkage, ODR, ABI, and libraries. Section 3 shows the Load stage of `SHARED` libraries with `readelf`, `dlopen`, and RPATH.
