# Playground

A scratch area for trying things out. CMake discovers each `*.c` and `*.cpp`
file directly in this directory and builds it as `pg_<file stem>`. Meson keeps
sources explicit, so add each new program to [meson.build](meson.build) as an
`executable()` target with the same naming convention.

```bash
cp hello.cpp try_variant.cpp     # write some code
make run EX=pg_try_variant       # build and run it in Docker
make meson-run EX=pg_try_variant  # after adding the Meson target
make run PRESET=asan EX=pg_try_variant   # same, with AddressSanitizer + UBSan
```

Playground programs are not registered as tests, so a crash or an infinite
loop here does not break `make test`.

When an experiment turns into a lesson, move it to the `examples/` directory
of the matching topic and register it with `add_example(...)` (see
[cmake/Examples.cmake](../cmake/Examples.cmake)).
