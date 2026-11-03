# Scratch area: every *.c / *.cpp file directly in this directory becomes its
# own executable named pg_<file stem>. Add a file, rebuild, run.
#
# The rest of the repository lists sources explicitly (see
# markdowns/Compile/02_modern_cmake.md). A glob is acceptable here only because this
# directory is a throwaway sandbox; CONFIGURE_DEPENDS re-runs it on each build.
file(GLOB playground_sources CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/*.c
     ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

foreach(source IN LISTS playground_sources)
    get_filename_component(stem ${source} NAME_WE)
    add_executable(pg_${stem} ${source})
    target_link_libraries(pg_${stem} PRIVATE example_warnings)
endforeach()
