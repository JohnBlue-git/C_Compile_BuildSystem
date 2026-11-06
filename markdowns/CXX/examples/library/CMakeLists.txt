# Examples for CXX/06-10 (language evolution, containers, OOP, streams, JSON).

# 06 language evolution
add_example(cxx_optional SOURCES optional.cpp)

# 07 containers
add_example(cxx_reference_wrapper SOURCES reference_wrapper.cpp)
add_example(cxx_emplace_back SOURCES emplace_back.cpp)
add_example(cxx_container_thread_safety SOURCES container_thread_safety.cpp
            LIBS Threads::Threads)

# 08 OOP
add_example(cxx_oop_encapsulation SOURCES oop/encapsulation.cpp)
add_example(cxx_oop_inheritance SOURCES oop/inheritance.cpp)
add_example(cxx_oop_polymorphism SOURCES oop/polymorphism.cpp)
add_example(cxx_oop_multi_inheritance SOURCES oop/multi_inheritance.cpp)
add_example(cxx_oop_singleton_cache SOURCES oop/singleton_cache.cpp)

# 09 streams (data.txt is copied next to each binary; the test runs there)
add_example(cxx_stream_state SOURCES stream/stream_state.cpp DATA data.txt)
add_example(cxx_stream_get_getline SOURCES stream/get_getline.cpp DATA data.txt)
add_example(cxx_stream_input_buffer SOURCES stream/input_buffer.cpp)
add_example(cxx_stream_ofstream_put SOURCES stream/ofstream_put.cpp)

# 10 JSON: optional dependencies, so the repo still configures without them.
find_package(nlohmann_json 3 CONFIG QUIET)
if(nlohmann_json_FOUND)
    add_example(cxx_json_nlohmann SOURCES json/nlohmann_basic.cpp
                LIBS nlohmann_json::nlohmann_json)
    add_example(cxx_json_nlohmann_pointer SOURCES json/nlohmann_pointer.cpp
                LIBS nlohmann_json::nlohmann_json)
else()
    message(STATUS "nlohmann_json not found: skipping cxx_json_nlohmann* examples")
endif()

find_package(Boost 1.83 CONFIG QUIET COMPONENTS json)
if(Boost_FOUND)
    add_example(cxx_json_boost SOURCES json/boost_basic.cpp LIBS Boost::json)
    add_example(cxx_json_boost_pointer SOURCES json/boost_pointer.cpp LIBS Boost::json)
else()
    message(STATUS "Boost.JSON not found: skipping cxx_json_boost* examples")
endif()
