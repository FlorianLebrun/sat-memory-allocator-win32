set(target sat-memory-allocator)

# Collect source files
append_group_sources(files FILTER "*.cc|*.cpp|*.h|*.hpp" ROOT "./src" DIRECTORIES 
  "./"
  "./allocator-LargeObject"
  "./allocator-ZonedBuddy"
  #"./allocator-Buddy"
  #"./allocator-ScaledBuddy"
  #"./allocator-ZonedPage"
  "./controller"
  "./controller/connector"
  "./controller/heaps"
  "./controller/profiler"
  "./controller/threads"
  "./heap-Compact"
  "./types"
  "./utils"

  # Win32 specific
  "./win32"
  "./win32/patcher"
)
append_group_sources(files FILTER "*.cc|*.cpp|*.h|*.hpp" DIRECTORIES 
  "./include/sat-memory-allocator"
  "./include/sat-memory-allocator/features"
)

# Add linker def file
list(APPEND files ${CMAKE_CURRENT_SOURCE_DIR}/library.def)

# Source includes
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Create library
add_library(${target} SHARED ${files})
set_target_properties(${target} PROPERTIES FOLDER "ThirdParty")
set_project_dll_properties(${target})

# Set compile properties
target_include_directories(${target} INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
