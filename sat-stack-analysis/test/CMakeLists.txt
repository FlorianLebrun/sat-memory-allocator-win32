set(target test-sat-stack-analysis)

# Collect source files
append_group_sources(files FILTER "*.c|*.cpp|*.h|*.hpp" DIRECTORIES "./")

# Create library
add_executable(${target} WIN32 ${files})
set_target_properties(${target} PROPERTIES FOLDER "Tests")

# Set link properties
target_link_libraries(${target} PRIVATE libsat)
target_link_options(${target} PRIVATE /SUBSYSTEM:CONSOLE)

# Setting for development comfort
if(DEVMODE AND SATMEM_POST_BUILD_DIRS)
set_target_postbuild_copy(${target} TO ${SATMEM_POST_BUILD_DIRS})
endif()
