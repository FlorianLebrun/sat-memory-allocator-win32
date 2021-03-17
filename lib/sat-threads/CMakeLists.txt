set(target sat-threads)

# Collect source files
append_group_sources(files FILTER "*.c|*.cpp|*.h|*.hpp" DIRECTORIES 
  "./src"
  "./src/worker"
)
append_group_sources(files FILTER "*.h|*.hpp" GROUP "Include" ROOT "./include" DIRECTORIES "./sat")

# Create library
add_library(${target} STATIC ${files})
set_target_properties(${target} PROPERTIES FOLDER "Libs")

# Set compile properties
target_include_directories(${target} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_definitions(${target} PRIVATE "SAT_API=__declspec(dllexport)")

# Set link properties
target_link_libraries(${target} PRIVATE boost sat-memory)
