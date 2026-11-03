# Examples for the C++ core notes CXX/00-05.
find_package(Threads REQUIRED)

# 00_core_fundamentals.md
add_example(cxx_raii SOURCES raii.cpp)
add_example(cxx_template SOURCES template.cpp)
add_example(cxx_crtp SOURCES crtp.cpp)
add_example(cxx_crtp_benchmark SOURCES crtp_benchmark.cpp)

# 01_smart_pointers.md
add_example(cxx_smart_pointers SOURCES smart_pointers.cpp)
add_example(cxx_weak_callback SOURCES weak_callback.cpp)

# 02_concurrency.md
add_example(cxx_async_future SOURCES async_future.cpp LIBS Threads::Threads)
add_example(cxx_promise_future SOURCES promise_future.cpp LIBS Threads::Threads)

# 03_move_semantics_and_rvo.md
add_example(cxx_move_semantics SOURCES move_semantics.cpp)
add_example(cxx_rvo_nrvo SOURCES rvo_nrvo.cpp)
# Same source with elision turned off where the standard allows it.
add_example(cxx_rvo_nrvo_no_elide SOURCES rvo_nrvo.cpp)
target_compile_options(cxx_rvo_nrvo_no_elide PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-fno-elide-constructors>)

# 04_lambdas_and_callbacks.md
add_example(cxx_lambdas SOURCES lambdas.cpp LIBS Threads::Threads)
add_example(cxx_callbacks SOURCES callbacks.cpp)

# 05_casts_and_exceptions.md
add_example(cxx_casts SOURCES casts.cpp)
add_example(cxx_exceptions SOURCES exceptions.cpp)
