set(target sat-allocator)

# Collect source files
append_group_sources(files FILTER "*.cc|*.cpp|*.h|*.hpp" ROOT "./src" DIRECTORIES 
  "./"

  "./allocators"
  "./allocators/LargeObject"
  "./allocators/ZonedBuddy"
  #"./allocators/Buddy"

  "./heaps"
  "./heaps/Compact"

  "./types"
  "./utils"

  # Win32 specific
  "./win32"
  "./win32/patcher"
)
append_group_sources(files FILTER "*.cc|*.cpp|*.h|*.hpp" GROUP "Include" ROOT "./include" DIRECTORIES 
  "./sat-memory-allocator"
)

# Source includes
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Create library
add_library(${target} STATIC ${files})
set_target_properties(${target} PROPERTIES FOLDER "Libs")

# Set compile properties
target_include_directories(${target} INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_definitions(${target} PRIVATE "SAT_API=__declspec(dllexport)")

# Set link properties
target_link_libraries(${target} PUBLIC sat-threads sat-memory)
