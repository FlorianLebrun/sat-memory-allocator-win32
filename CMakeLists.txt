cmake_minimum_required(VERSION 3.16)

project("sat-memory-allocator" 
  LANGUAGES ASM_MASM C CXX
  VERSION 1.0.0
)

# Include cmake tools
set(NODE_TOOLS "${CMAKE_SOURCE_DIR}/node_modules/tools")
if(NOT EXISTS ${NODE_TOOLS})
execute_process(COMMAND cmd /c npm install WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endif()
include(${NODE_TOOLS}/index.cmake)

# Set Global compilation options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W0")

# Add projects
add_subdirectory("./library")
add_subdirectory("./test")
