set(target libsat)

# Create library
add_library(${target} SHARED main.cpp)
set_target_properties(${target} PROPERTIES FOLDER "Dll")

# Set compile properties
target_compile_definitions(${target} PRIVATE "SAT_API=__declspec(dllexport)")
target_compile_definitions(${target} INTERFACE "SAT_API=__declspec(dllimport)")

# Set link properties
target_link_libraries(${target} PRIVATE sat-threads sat-memory sat-allocator sat-stack-analysis)
target_link_options(${target} PRIVATE /WHOLEARCHIVE:sat-threads /WHOLEARCHIVE:sat-memory /WHOLEARCHIVE:sat-allocator /WHOLEARCHIVE:sat-stack-analysis)

# Export interface of libraries linked inside this shared library
target_include_directories(${target} INTERFACE $<TARGET_PROPERTY:libsat,INCLUDE_DIRECTORIES>)

# Setting for development comfort
if(DEVMODE AND SATMEM_POST_BUILD_DIRS)
set_target_postbuild_copy(${target} TO ${SATMEM_POST_BUILD_DIRS})
endif()
