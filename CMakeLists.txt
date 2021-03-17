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
add_compile_definitions(WINVER=0x0600)
add_compile_definitions(_WIN32_WINNT=0x0600)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3")

# Setting for development comfort
set(SATMEM_POST_BUILD_DIRS ${PROJECT_DEV_DIR})

# Add projects
add_subdirectory("./thirdparty/boost")
add_subdirectory("./lib")
