cmake_minimum_required(VERSION 3.25)
project(CppInterviewPlayground LANGUAGES C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

enable_testing()
include(cmake/Examples.cmake)

add_subdirectory(markdowns/C/examples)
add_subdirectory(markdowns/CXX/examples)
add_subdirectory(markdowns/Compile/examples)
add_subdirectory(markdowns/Memory/examples)
add_subdirectory(playground)
