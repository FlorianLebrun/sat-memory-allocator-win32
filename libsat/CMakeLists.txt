set(target libsat)

# Collect source files
append_group_sources(files FILTER "*.c|*.cpp|*.h|*.hpp" DIRECTORIES ".")

# Add linker def file
list(APPEND files ${CMAKE_CURRENT_SOURCE_DIR}/library.def)

# Create library
add_library(${target} SHARED main.cpp)
set_target_properties(${target} PROPERTIES FOLDER "Dll")

target_link_libraries(${target} PRIVATE sat-threads sat-memory sat-allocator)
