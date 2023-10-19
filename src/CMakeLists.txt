file(GLOB_RECURSE srcs "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB_RECURSE headers "${CMAKE_CURRENT_SOURCE_DIR}/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp")
add_library(NatarajanBST ${srcs} ${headers})
# this makes the headers accessible for other projects which uses NatarajanBST lib
target_include_directories(NatarajanBST PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/NatarajanBST")
