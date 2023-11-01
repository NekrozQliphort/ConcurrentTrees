cmake_minimum_required(VERSION 3.26)
project(ConcurrentBST)

option(WITH_TSAN "Build tests and benchmarks with ThreadSanitizer" ON)

set(CMAKE_VERBOSE_MAKEFILE on)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED on)

include_directories("${CMAKE_CURRENT_LIST_DIR}/lib") #include catch.hpp

file(GLOB_RECURSE CONCURRENT_TREE_FILES src/*.h)
file(GLOB_RECURSE TEST_FILES tests/*.h tests/*.cpp)

include_directories(.)

add_executable(test ${TEST_FILES} ${CONCURRENT_TREE_FILES})

if(WITH_TSAN AND NOT MSVC)
    string(APPEND CMAKE_CXX_FLAGS " -fsanitize=thread")
endif()