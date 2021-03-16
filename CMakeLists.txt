cmake_minimum_required(VERSION 3.19)

project("sat-memory-allocator" 
  LANGUAGES ASM_MASM C CXX
  VERSION 1.0.0
)

# Include cmake tools
set(NODE_TOOLS "${CMAKE_SOURCE_DIR}/node_modules/script.cmake")
if(NOT EXISTS ${NODE_TOOLS})
execute_process(COMMAND cmd /c npm install WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endif()
include(${NODE_TOOLS}/index.cmake)

# Set Global compilation options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W0")

# Setting for development comfort
set(SATMEM_POST_BUILD_DIRS ${PROJECT_DEV_DIR})

# Add projects
add_subdirectory("./thirdparty/boost")
add_subdirectory("./sat-memory")
add_subdirectory("./sat-allocator")
add_subdirectory("./sat-stack-analysis")
add_subdirectory("./sat-threads")
add_subdirectory("./libsat")

# Add tests projects
add_subdirectory("./sat-memory/test")
add_subdirectory("./sat-allocator/test")
add_subdirectory("./sat-stack-analysis/test")
add_subdirectory("./sat-threads/test")
