set(target test)

# Collect source files
append_group_sources(files FILTER "*.c|*.cpp|*.h|*.hpp" DIRECTORIES "./")

# Create library
add_executable(${target} WIN32 ${files})
add_dependencies(${target} sat-memory-allocator)
set_target_properties(${target} PROPERTIES FOLDER "Test")
set_project_exe_properties(${target})

# Set link properties
target_link_libraries(${target} PRIVATE sat-memory-allocator)
target_link_options(${target} PRIVATE /SUBSYSTEM:CONSOLE)
