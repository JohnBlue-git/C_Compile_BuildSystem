# A separate, minimal target-based project. lab.sh configures, builds, and
# installs it to show the build-tree RUNPATH and the install RUNPATH.
cmake_minimum_required(VERSION 3.25)
project(RpathDemo LANGUAGES CXX)

add_library(mathcxx SHARED src/mathcxx.cpp)
target_include_directories(mathcxx PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_compile_features(mathcxx PUBLIC cxx_std_20)

add_executable(app src/main.cpp)
target_compile_definitions(app PRIVATE APP_VERSION=1)
target_link_libraries(app PRIVATE mathcxx) # also gets the include dir (PUBLIC)
set_target_properties(app PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")

install(TARGETS app mathcxx RUNTIME DESTINATION bin LIBRARY DESTINATION lib)
install(FILES include/mathcxx.h DESTINATION include)
