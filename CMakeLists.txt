cmake_minimum_required(VERSION 3.16)

project("sat-memory-allocator" 
  LANGUAGES ASM_MASM C CXX
  VERSION 1.0.0
)

include(.tools/index.cmake)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W0")

add_subdirectory("./library")
add_subdirectory("./test")
