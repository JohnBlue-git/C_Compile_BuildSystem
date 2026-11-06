set(EXAMPLE_LABEL cxx)

# Imported targets are directory-scoped, so find Threads here for core/ and library/.
find_package(Threads REQUIRED)

add_subdirectory(core)
add_subdirectory(library)
