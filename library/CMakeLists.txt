set(target sat-memory-allocator)

# Collect source files
append_group_sources(files FILTER "*.cc|*.cpp|*.h|*.hpp" ROOT "./src" DIRECTORIES 
  "./"

  "./allocators"
  "./allocators/LargeObject"
  "./allocators/ZonedBuddy"
  #"./allocators/Buddy"
  #"./allocators/ScaledBuddy"
  #"./allocators/ZonedPage"

  "./heaps"
  "./heaps/Compact"

  "./controller"
  "./controller/connector"
  "./controller/profiler"
  "./controller/threads"

  "./types"
  "./utils"

  # Win32 specific
  "./win32"
  "./win32/patcher"
)
append_group_sources(files FILTER "*.cc|*.cpp|*.h|*.hpp" DIRECTORIES 
  "./include/sat-memory-allocator"
)

# Add linker def file
list(APPEND files ${CMAKE_CURRENT_SOURCE_DIR}/library.def)

# Source includes
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Create library
add_library(${target} SHARED ${files})
set_target_properties(${target} PROPERTIES FOLDER "ThirdParty")

# Set compile properties
target_include_directories(${target} INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

# Setting for development comfort
if(DEVMODE AND SATMEM_POST_BUILD_DIRS)
set_target_postbuild_copy(${target} TO ${SATMEM_POST_BUILD_DIRS})
endif()
