# gcc/g++ and CMake Basics

This note follows [C/C++ Compilation and Linking](00_compile_and_linkage.md). It compares running `gcc`/`g++` by hand with letting CMake manage the same build graph, and it covers both CMake styles: **non-target-based (traditional, directory-scoped)** and **target-based (modern)**. CMake is not a compiler. It generates build rules for Make, Ninja, or an IDE, and gcc, g++, or another toolchain still does the preprocessing, compiling, and linking. Loading `.so` files at run time is the job of the OS dynamic loader, whatever CMake style you use.

## 1. gcc/g++ vs CMake: who does what

| Task | GNU toolchain by hand | CMake (target-based) |
|---|---|---|
| Sources | `gcc main.c math.c` | `add_executable(app main.c math.c)` |
| C standard | `gcc -std=c17` | `target_compile_features(app PRIVATE c_std_17)` |
| C++ standard | `g++ -std=c++20` | `target_compile_features(app PRIVATE cxx_std_20)` |
| Include path | `-Iinclude` | `target_include_directories(app PRIVATE include)` |
| Library | `-Llib -lmath` | `target_link_libraries(app PRIVATE math)` |
| Compile flags | `-Wall -Wextra` | `target_compile_options(app PRIVATE -Wall -Wextra)` |
| Link flags | `-pthread`, `-fsanitize=address` | `target_link_options(app PRIVATE ...)` or an imported target (`Threads::Threads`) |

## 2. gcc vs g++

- `gcc` usually compiles C, and `g++` usually compiles C++. When `g++` links, it also adds the C++ standard library (`-lstdc++ -lm`).
- `gcc` can compile a `.cpp` file too (it picks the language from the file extension). But if `gcc` does the final link, `libstdc++` is usually missing. C++ executables use `g++` as the link driver.
- `gcc` and `g++` do not use different linkers. Both call the same `collect2`/`ld` with the same startup files. The differences are the language defaults and the libraries that the driver adds.
- In a mixed C/C++ project, `gcc` compiles the `.c` files, `g++` compiles the `.cpp` files, `g++` links, and `extern "C"` handles the C ABI.
- **This difference only exists at Compile and Link** (whether `libstdc++` is added). The Load stage (can the loader find the `.so` at run time?) is a different question. It depends only on the dependencies and `RUNPATH` recorded in the final binary, not on gcc vs g++. Section 4 compares this with CMake's behavior.

```sh
gcc -std=c17   -Wall -Wextra -c math.c -o math.o
g++ -std=c++20 -Wall -Wextra -c main.cpp -o main.o
g++ main.o math.o -o app       # g++ does the final C++ link
```

> The real gcc/g++ output for all of this is in [00, section 2](00_compile_and_linkage.md#2-c-vs-c-at-compile-and-link-time-a-gccg-demo) and in the `compile_pipeline_lab`.

## 3. Two CMake styles: non-target-based (traditional) vs target-based (modern)

### 3.1 Non-target-based: directory-scoped commands

This was the common style before CMake 3.0. The commands apply to **the whole directory of the `CMakeLists.txt`** (and its subdirectories), not to one target. They affect every target created **after** the command in that directory. Common commands:

| Command | Stage | Effect |
|---|---|---|
| `include_directories(dir)` | Include/Compile | Adds `-Idir` to **every** later target in this directory |
| `add_definitions(-DFOO)` | Compile | Adds the macro to every later target in this directory |
| `set(CMAKE_C_FLAGS ...)` / `set(CMAKE_CXX_FLAGS ...)` | Compile | Global compiler flags |
| `link_directories(dir)` | Link | Adds `-Ldir` to every later target in this directory |
| `link_libraries(lib)` | Link | Links `lib` into **every** later target in this directory |
| `install(TARGETS ...)` / `install(FILES ...)` | Install | Always worked per target/file, so the two styles hardly differ here |

### 3.2 A minimal non-target-based project: Include / Compile / Link / Load / Install

Project layout:

```
.
├── CMakeLists.txt
├── include/
│   └── mathcxx.h
└── src/
    ├── mathcxx.cpp
    └── main.cpp
```

```cmake
cmake_minimum_required(VERSION 3.20)
project(LegacyCMakeDemo LANGUAGES CXX)

# --- Include: a global include path for every later target in this directory ---
include_directories(include)

# --- Compile: global compiler flags and macros ---
add_definitions(-DAPP_VERSION=1)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# --- Load: a global install RPATH. Set it BEFORE creating the targets: it only
#     initializes the INSTALL_RPATH property of targets created after this line.
set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")

add_library(mathcxx SHARED src/mathcxx.cpp)

# --- Link: a global library list. It must come AFTER add_library(mathcxx);
#     before it, mathcxx itself would get the library, and CMake stops with
#     "Target "mathcxx" links to itself."
link_libraries(mathcxx)

add_executable(app src/main.cpp)   # gets -Iinclude, -DAPP_VERSION=1, and mathcxx

# --- Install ---
install(TARGETS app mathcxx
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)
install(FILES include/mathcxx.h DESTINATION include)
```

Two ordering traps are visible here (both verified with CMake 3.28):

- `link_libraries(mathcxx)` before `add_library(mathcxx ...)` fails at generate time: `Target "mathcxx" links to itself.`
- `set(CMAKE_INSTALL_RPATH ...)` after `add_executable(app ...)` does nothing for `app`. The installed binary has no `RUNPATH` and fails with `libmathcxx.so: cannot open shared object file`. Directory-scoped variables are read when a target is **created**.

`link_directories()` is not needed here. CMake links in-project targets by their full path.

```sh
cmake -S . -B build
cmake --build build --verbose      # shows the real g++ commands

./build/app                        # Load (build tree): CMake wrote a build-tree RUNPATH, so it runs

cmake --install build --prefix ./stage
./stage/bin/app                    # Load (installed): uses CMAKE_INSTALL_RPATH from above, so it runs
```

The verbose build shows that the directory-scoped settings reach **every** target. The library also gets `-DAPP_VERSION=1` and `-Iinclude`, although it only needed the include path. (`<src>` stands for the source directory.)

```text
/usr/bin/c++ -DAPP_VERSION=1 -Dmathcxx_EXPORTS -I<src>/include -Wall -Wextra -std=gnu++20 -fPIC ... -c <src>/src/mathcxx.cpp
/usr/bin/c++ -fPIC  -Wall -Wextra -shared -Wl,-soname,libmathcxx.so -o libmathcxx.so CMakeFiles/mathcxx.dir/src/mathcxx.cpp.o ...
/usr/bin/c++ -DAPP_VERSION=1 -I<src>/include -Wall -Wextra -std=gnu++20 ... -c <src>/src/main.cpp
/usr/bin/c++  -Wall -Wextra CMakeFiles/app.dir/src/main.cpp.o -o app  -Wl,-rpath,<build>: libmathcxx.so
```

Also note `-std=gnu++20`: `CMAKE_CXX_STANDARD 20` turns on the GNU dialect unless you also set `CMAKE_CXX_EXTENSIONS OFF` (this repo's root `CMakeLists.txt` does).

After install:

```text
-- Set non-toolchain portion of runtime path of "./stage/bin/app" to "$ORIGIN/../lib"
$ readelf -d stage/bin/app | grep RUNPATH
 0x000000000000001d (RUNPATH)            Library runpath: [$ORIGIN/../lib]
$ ./stage/bin/app
42 1
```

### 3.3 The problem with non-target-based CMake: directory scope leaks

`include_directories()` and `link_libraries()` are **global**. Every later target in the same `CMakeLists.txt` gets them, even when it does not need them:

```cmake
add_executable(other_tool src/other_tool.cpp)
# other_tool does not need mathcxx, but because of link_libraries(mathcxx)
# it links it anyway, and it also gets -Iinclude. Neither is related to it.
```

That is the main reason CMake has recommended target-based commands since 3.x. `target_include_directories` and `target_link_libraries` act only on the named target. `PRIVATE`/`PUBLIC`/`INTERFACE` decide whether a setting passes on to the targets that link it, so unrelated targets are not affected.

The target-based version of the same project is [examples/cmake_rpath/CMakeLists.txt](examples/cmake_rpath/CMakeLists.txt). The library declares its include directory `PUBLIC`, `app` links it `PRIVATE`, and only `app` gets `INSTALL_RPATH`:

> Run it: `make shell`, then `bash markdowns/Compile/examples/cmake_rpath/lab.sh` (CTest: `compile_cmake_rpath_lab`).

```text
=== 1. Configure and build; the verbose log shows the real g++ commands ===
$ cmake --build build --verbose | grep -o "c++ .*"
c++ -Dmathcxx_EXPORTS -I<src>/include -O3 -DNDEBUG -std=gnu++20 -fPIC -MD -MT ... -c <src>/src/mathcxx.cpp
c++ -DAPP_VERSION=1 -I<src>/include -O3 -DNDEBUG -std=gnu++20 -MD -MT ... -c <src>/src/main.cpp
c++ -fPIC -O3 -DNDEBUG   -shared -Wl,-soname,libmathcxx.so -o libmathcxx.so CMakeFiles/mathcxx.dir/src/mathcxx.cpp.o   && :
c++ -O3 -DNDEBUG  CMakeFiles/app.dir/src/main.cpp.o -o app  -Wl,-rpath,<work>/build:  libmathcxx.so && :

=== 2. Build tree: CMake writes the build directory into RUNPATH ===
$ readelf -d build/app | grep RUNPATH
 0x000000000000001d (RUNPATH)            Library runpath: [/tmp/tmp.tBKoSIDRUa/build:]
$ ./build/app
multiply(6, 7) = 42, APP_VERSION = 1

=== 3. Install: CMake replaces it with INSTALL_RPATH ===
$ cmake --install build --prefix stage
-- Install configuration: "Release"
-- Installing: /tmp/tmp.tBKoSIDRUa/stage/bin/app
-- Set non-toolchain portion of runtime path of "stage/bin/app" to "$ORIGIN/../lib"
-- Installing: /tmp/tmp.tBKoSIDRUa/stage/lib/libmathcxx.so
-- Installing: /tmp/tmp.tBKoSIDRUa/stage/include/mathcxx.h
$ readelf -d stage/bin/app | grep RUNPATH
 0x000000000000001d (RUNPATH)            Library runpath: [$ORIGIN/../lib]
$ ./stage/bin/app
multiply(6, 7) = 42, APP_VERSION = 1
```

Now `-DAPP_VERSION=1` reaches only `main.cpp`, and the library gets no `RUNPATH` at install. Compare this with the legacy build above.

## 4. Every stage side by side: gcc/g++ vs CMake

| Stage | gcc/g++ by hand | CMake: non-target-based | CMake: target-based |
|---|---|---|---|
| Include path | `-Iinclude` | `include_directories(include)` (global) | `target_include_directories(app PRIVATE include)` (only `app`) |
| Compile macros/flags | `-DAPP_VERSION=1 -Wall` | `add_definitions(...)`, `set(CMAKE_CXX_FLAGS ...)` (global) | `target_compile_definitions`, `target_compile_options` (per target, `PRIVATE`/`PUBLIC`) |
| C/C++ standard | `-std=c17` / `-std=c++20` | `set(CMAKE_C_STANDARD 17)` / `CMAKE_CXX_STANDARD 20` (default for later targets) | `target_compile_features(app PRIVATE cxx_std_20)` (per target) |
| Library search path | `-Llib` | `link_directories(lib)` (global) | Usually not needed: CMake uses full paths for targets, or an imported target |
| Library list | `-lmathcxx` | `link_libraries(mathcxx)` (global, every later target) | `target_link_libraries(app PRIVATE mathcxx)` (only `app`) |
| Link driver (gcc vs g++) | You choose; easy to forget g++ in mixed projects | CMake chooses from the source languages (`LINKER_LANGUAGE`), in both styles | Same |
| Load (run from the build tree) | Manual `-Wl,-rpath` or `LD_LIBRARY_PATH` | CMake writes a build-tree `RUNPATH` by default (`CMAKE_SKIP_BUILD_RPATH` is `OFF`), in both styles | Same |
| Load (after install) | Deployment script / `-Wl,-rpath,...` | `set(CMAKE_INSTALL_RPATH ...)` before the targets (initializes every later target) | `set_target_properties(app PROPERTIES INSTALL_RPATH ...)` (one target) |
| Install | `cp` / a hand-written `install:` rule | `install(TARGETS ...)`, `install(FILES ...)` | Same: `install()` always worked per target/file |

## 5. Summary: the real differences

1. **gcc vs g++** differs only in driver behavior at Compile and Link (is `libstdc++` added?). CMake style does not matter here. Both styles still call gcc/g++, and CMake picks the right driver from the languages.
2. **Non-target-based vs target-based CMake** does not differ in *what* it can do: both can handle include/link/load/install. The difference is **scope**. Directory-scoped commands leak into unrelated targets (and depend on the order of commands). Target-based commands use `PRIVATE`/`PUBLIC`/`INTERFACE` to control each target's include/compile/link settings exactly.
3. **Load** has two cases in CMake. In the build tree, CMake handles it by default (both styles). After install, CMake removes the build-tree path, and you must set `CMAKE_INSTALL_RPATH` or `INSTALL_RPATH` yourself. That is CMake's RPATH policy, not a style question. It is the same problem that `-Wl,-rpath` solves for manual `gcc`/`g++`. CMake wraps it, but by default only for the build tree.

## Further reading

- [Modern CMake](02_modern_cmake.md): target-based CMake, dependencies, sanitizers, installing, common traps, and this repo's own build as an example.
- [C/C++ Compilation and Linking](00_compile_and_linkage.md): translation units, linkage, ODR, ABI, and libraries.
