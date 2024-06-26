cmake_minimum_required(VERSION 3.22)
project(ConcurrentBST)

include(FetchContent)

option(WITH_TSAN "Build tests and benchmarks with ThreadSanitizer" OFF)

set(CMAKE_VERBOSE_MAKEFILE on)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED on)
string(APPEND CMAKE_CXX_FLAGS_RELEASE " -DNDEBUG -O3")

FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.13.x
)
FetchContent_Declare(
        googlebenchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG main
)
FetchContent_MakeAvailable(googletest googlebenchmark)

include_directories("${CMAKE_CURRENT_LIST_DIR}/lib")

file(GLOB_RECURSE CONCURRENT_TREE_FILES src/*.h)
file(GLOB_RECURSE TEST_FILES tests/*.h tests/*.cpp)
file(GLOB_RECURSE BENCHMARK_FILES benchmark/*.h benchmark/*.cpp)

include_directories(.)

add_executable(bsttest ${TEST_FILES} ${CONCURRENT_TREE_FILES})

foreach( benchmarkfile ${BENCHMARK_FILES} )
    get_filename_component( benchmarkname ${benchmarkfile} NAME_WE )
    add_executable( ${benchmarkname} ${benchmarkfile} ${CONCURRENT_TREE_FILES})
    target_link_libraries( ${benchmarkname} benchmark::benchmark )
endforeach( benchmarkfile ${BENCHMARK_FILES} )

if(WITH_TSAN AND NOT MSVC)
    MESSAGE(STATUS "Compiling with thread sanitizer")
    string(APPEND CMAKE_CXX_FLAGS " -fsanitize=thread -pie -fPIE")
endif()