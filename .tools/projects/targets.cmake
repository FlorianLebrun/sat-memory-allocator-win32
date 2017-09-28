

# Set default project properties
set_property(GLOBAL PROPERTY USE_FOLDERS TRUE)
set(PROJECT_OUTPUT_DIR ${CMAKE_SOURCE_DIR}/.output)
set(PROJECT_OUTPUT_BUILD_DIR ${PROJECT_OUTPUT_DIR}/build)
if(NOT PACKAGES_OUTPUT_DIR)
set(PACKAGES_OUTPUT_DIR ${PROJECT_OUTPUT_DIR}/packages)
endif()

# Set build multiprocessor
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP") # Build multiprocessor

macro(set_project_common_output target outputDir suffix)
  set(buildDir "${CMAKE_CURRENT_BINARY_DIR}/bin")

  set_target_properties(${target} PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${buildDir}
    LIBRARY_OUTPUT_DIRECTORY ${buildDir}
    RUNTIME_OUTPUT_DIRECTORY ${buildDir}
    COMPILE_PDB_OUTPUT_DIRECTORY ${buildDir}
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${buildDir}
    LIBRARY_OUTPUT_DIRECTORY_DEBUG ${buildDir}
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${buildDir}
    COMPILE_PDB_OUTPUT_DIRECTORY_DEBUG ${buildDir}
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${buildDir}
    LIBRARY_OUTPUT_DIRECTORY_RELEASE ${buildDir}
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${buildDir}
    COMPILE_PDB_OUTPUT_DIRECTORY_RELEASE ${buildDir}
  )

  add_custom_command(TARGET ${target} POST_BUILD
    COMMAND node "${CMAKE_SOURCE_DIR}/.tools/packaging/copy-file" --input ${buildDir}/${target}${suffix} --destination ${outputDir}
    COMMAND node "${CMAKE_SOURCE_DIR}/.tools/packaging/copy-file" --input ${buildDir}/${target}.pdb --destination ${outputDir}
    COMMAND node "${CMAKE_SOURCE_DIR}/.tools/packaging/copy-file" --input ${buildDir}/${target}.lib --destination ${outputDir}
  )

endmacro()


macro(set_project_dll_properties target)

  # Set output properties
  set_project_common_output(${target} "${PROJECT_OUTPUT_BUILD_DIR}/Dll" ".dll")

  # Set version includes
  target_include_directories(${target} PRIVATE ${CMAKE_BINARY_DIR})

endmacro()


macro(set_project_exe_properties target)

  # Set output properties
  set_project_common_output(${target} "${PROJECT_OUTPUT_BUILD_DIR}/Bin" ".exe")

  # Set version includes
  target_include_directories(${target} PRIVATE ${CMAKE_BINARY_DIR})
  
  # set debug env
  set_target_properties(${target} PROPERTIES 
    VS_DEBUGGER_ENVIRONMENT "PATH=%PATH%;${PROJECT_OUTPUT_BUILD_DIR}/Dll"
  )

endmacro()

