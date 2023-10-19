cmake_minimum_required(VERSION 3.26)
project(ConcurrentBST)

set(CMAKE_CXX_STANDARD 20)

add_executable(ConcurrentTrees main.cpp)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/src")
target_include_directories(ConcurrentTrees PUBLIC
        "${PROJECT_BINARY_DIR}"
        "${PROJECT_SOURCE_DIR}/NatarajanBST"
)
target_link_libraries(ConcurrentTrees PUBLIC NatarajanBST)