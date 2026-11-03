set(EXAMPLE_LABEL compile)

# Shell labs: they run gcc/g++/nm/readelf (and cmake) directly, so the commands
# are visible.
# Each writes its files to a directory in the build tree.
foreach(lab IN ITEMS pipeline linkage libraries cmake_rpath)
    add_test(NAME compile_${lab}_lab
             COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/${lab}/lab.sh
                     ${CMAKE_CURRENT_BINARY_DIR}/${lab}_lab)
    set_tests_properties(compile_${lab}_lab PROPERTIES TIMEOUT 60 LABELS compile)
endforeach()

# Mixed C/C++ project: add.c is compiled as C, main.cpp as C++, linked by g++.
add_example(compile_mixed_c_cxx SOURCES mixed/main.cpp mixed/add.c)

# dlopen/dlsym: the same function in two plugins, with and without extern "C".
add_library(compile_plugin_mangled SHARED dlopen/add.cpp)
add_library(compile_plugin_extern_c SHARED dlopen/add_c_linkage.cpp)
foreach(plugin IN ITEMS compile_plugin_mangled compile_plugin_extern_c)
    target_link_libraries(${plugin} PRIVATE example_warnings)
endforeach()

add_example(compile_dlopen_loader SOURCES dlopen/loader.c LIBS ${CMAKE_DL_LIBS}
            ARGS $<TARGET_FILE:compile_plugin_mangled> $<TARGET_FILE:compile_plugin_extern_c>)
# The loader does not link the plugins, so build them with it (for make run).
add_dependencies(compile_dlopen_loader compile_plugin_mangled compile_plugin_extern_c)
