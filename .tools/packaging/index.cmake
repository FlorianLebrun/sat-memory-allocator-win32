

function(append_package_signed_target_copy LIST target destionationDir)
  cmake_parse_arguments(ARG "" "" "DEPENDS" ${ARGN})
  get_target_property(type ${target} TYPE)
  if(type STREQUAL "EXECUTABLE")
    set(suffix ".exe")
  else()
    set(suffix ".dll")
  endif()
  get_target_property(sourceDir ${target} RUNTIME_OUTPUT_DIRECTORY)
  append_package_signed_copy(files "${target}${suffix}" ${sourceDir} ${destionationDir} DEPENDS ${target})
  append_package_copy(files "${target}.pdb" ${sourceDir} ${destionationDir} DEPENDS ${target})
  set(${LIST} ${${LIST}} ${files} PARENT_SCOPE)
endfunction()


function(append_package_copy LIST filename sourceDir destinationDir)
  cmake_parse_arguments(ARG "" "" "DEPENDS" ${ARGN})
  set(inputFile "${sourceDir}/${filename}")
  set(outputFile "${destinationDir}/${filename}")
  add_custom_command(
    OUTPUT ${outputFile}
    DEPENDS ${inputFile} ${ARG_DEPENDS}
    COMMAND node "${CMAKE_SOURCE_DIR}/.tools/packaging/copy-file"
      --input ${inputFile}
      --output ${outputFile}
  )
  set(${LIST} ${${LIST}} ${outputFile} PARENT_SCOPE)
endfunction()


function(append_package_signed_copy LIST filename sourceDir destinationDir)
  cmake_parse_arguments(ARG "" "" "DEPENDS" ${ARGN})
  set(inputFile "${sourceDir}/${filename}")
  set(outputFile "${destinationDir}/${filename}")
  add_custom_command(
    OUTPUT ${outputFile}
    DEPENDS ${inputFile} ${ARG_DEPENDS}
    COMMAND node "${CMAKE_SOURCE_DIR}/.tools/packaging/copy-sign-file"
      --input ${inputFile}
      --output ${outputFile}
  )
  set(${LIST} ${${LIST}} ${outputFile} PARENT_SCOPE)
endfunction()


function(append_package_pack LIST packageTag sourceDir destinationDir)
  cmake_parse_arguments(ARG "" "" "DEPENDS" ${ARGN})
  set(outputFile "${sourceDir}/${packageTag}.log")
  add_custom_command(
    OUTPUT ${outputFile}
    DEPENDS ${ARG_DEPENDS}
    WORKING_DIRECTORY "${sourceDir}"
    COMMAND node "${CMAKE_SOURCE_DIR}/.tools/packaging/pack-directory"
      --name "${packageTag}"
      --version "${CMAKE_PROJECT_VERSION}-${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}"
      --debug $<CONFIG:Debug>
      --output "${destinationDir}"
    > ${outputFile}
  )
  set(${LIST} ${${LIST}} ${outputFile} PARENT_SCOPE)
endfunction()

