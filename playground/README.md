# Playground

A scratch area for trying things out. Every `*.c` and `*.cpp` file directly in
this directory becomes its own executable named `pg_<file stem>`. You don't
need to edit any CMake file.

```bash
cp hello.cpp try_variant.cpp     # write some code
make run EX=pg_try_variant       # build and run it in Docker
make run PRESET=asan EX=pg_try_variant   # same, with AddressSanitizer + UBSan
```

Playground programs are not registered as tests, so a crash or an infinite
loop here does not break `make test`.

When an experiment turns into a lesson, move it to the `examples/` directory
of the matching topic and register it with `add_example(...)` (see
[cmake/Examples.cmake](../cmake/Examples.cmake)).
