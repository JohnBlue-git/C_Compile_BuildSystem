# Shared helper for every example in the repository.
#
# add_example(<name>
#             SOURCES <file>...
#             [LIBS <target>...]     extra libraries to link
#             [DATA <file>...]       files copied next to the binary; the test runs there
#             [ARGS <arg>...]        command-line arguments for the test run
#             [NO_TEST]              build only (for example, interactive programs)
#             [WILL_FAIL])           the test passes when the program fails (crash demos)
#
# Each examples/ directory sets EXAMPLE_LABEL (c, cxx, compile, memory), so
# `ctest --preset debug -L cxx` runs only that topic.

add_library(example_warnings INTERFACE)
target_compile_options(example_warnings INTERFACE
    $<$<OR:$<C_COMPILER_ID:GNU,Clang>,$<CXX_COMPILER_ID:GNU,Clang>>:-Wall -Wextra -Wpedantic>)

function(add_example name)
    cmake_parse_arguments(PARSE_ARGV 1 EX "NO_TEST;WILL_FAIL" "" "SOURCES;LIBS;DATA;ARGS")

    add_executable(${name} ${EX_SOURCES})
    target_link_libraries(${name} PRIVATE example_warnings ${EX_LIBS})

    foreach(file IN LISTS EX_DATA)
        configure_file(${file} ${CMAKE_CURRENT_BINARY_DIR}/${file} COPYONLY)
    endforeach()

    if(NOT EX_NO_TEST)
        add_test(NAME ${name}
                 COMMAND ${name} ${EX_ARGS}
                 WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
        set_tests_properties(${name} PROPERTIES TIMEOUT 30 LABELS "${EXAMPLE_LABEL}")
        if(EX_WILL_FAIL)
            set_tests_properties(${name} PROPERTIES WILL_FAIL TRUE)
        endif()
    endif()
endfunction()
