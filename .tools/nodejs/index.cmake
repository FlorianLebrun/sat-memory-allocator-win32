
include(.tools/nodejs/NodeJS.cmake)

function(append_npm_build LIST)
  cmake_parse_arguments(ARG "" "SOURCE;DESTINATION" "DIRECTORIES;DEPENDS" ${ARGN})
  set(outputFile "${CMAKE_CURRENT_BINARY_DIR}/nodejs-build.log")

  append_group_sources(ts_sources FILTER "*.ts|*.tsx" DIRECTORIES ${ARG_DIRECTORIES})

  add_custom_command(
    OUTPUT ${outputFile}
    DEPENDS ${ARG_TSCONFIG} ${ts_sources} ${ARG_DEPENDS}
    WORKING_DIRECTORY ${node_script}
    COMMAND node "${CMAKE_SOURCE_DIR}/.tools/nodejs/make_build"
      --source "${ARG_SOURCE}"
      --destination "${ARG_DESTINATION}"
    COMMAND ${CMAKE_COMMAND} -E echo "done" > ${outputFile}
  )
  set(${LIST} ${${LIST}} ${outputFile} PARENT_SCOPE)
endfunction()

function(append_npm_package_json LIST packageTag)
  cmake_parse_arguments(ARG "" "INSTALL;SOURCE;DESTINATION" "DEPENDS" ${ARGN})
  set(outputFile "${ARG_DESTINATION}/package.json")
  add_custom_command(
    OUTPUT ${outputFile}
    DEPENDS ${ARG_SOURCE}/package.json ${ARG_DEPENDS}
    COMMAND node "${CMAKE_SOURCE_DIR}/.tools/nodejs/make_package_json"
      --name "${packageTag}"
      --source "${ARG_SOURCE}"
      --destination "${ARG_DESTINATION}"
      --install ${ARG_INSTALL}
      --version ${CMAKE_PROJECT_VERSION}
  )
  set(${LIST} ${${LIST}} ${outputFile} PARENT_SCOPE)
endfunction()
