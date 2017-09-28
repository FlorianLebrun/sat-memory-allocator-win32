
## Get all properties that cmake supports
execute_process(COMMAND ${CMAKE_COMMAND} --help-property-list OUTPUT_VARIABLE CMAKE_PROPERTY_LIST)
## Convert command output into a CMake list
STRING(REGEX REPLACE ";" "\\\\;" CMAKE_PROPERTY_LIST "${CMAKE_PROPERTY_LIST}")
STRING(REGEX REPLACE "\n" ";" CMAKE_PROPERTY_LIST "${CMAKE_PROPERTY_LIST}")

list(REMOVE_DUPLICATES CMAKE_PROPERTY_LIST)

function(print_target_properties target)
  if(NOT TARGET ${target})
    message("There is no target named '${target}'")
    return()
  endif()

  cmake_policy(PUSH)
  cmake_policy(SET CMP0026 OLD)
  foreach (prop ${CMAKE_PROPERTY_LIST})
      string(REPLACE "<CONFIG>" "${CMAKE_BUILD_TYPE}" prop ${prop})
      get_target_property(propval ${target} ${prop})
      if (propval)
          get_target_property(propval ${target} ${prop})
          message ("[${target}] ${prop} = ${propval}")
      endif()
  endforeach(prop)
  cmake_policy(POP)
endfunction(print_target_properties)

function(print_variables)
  get_cmake_property(L_names VARIABLES)
  foreach (L_variable ${L_names})
    message(STATUS "[VARIABLE] ${L_variable} = ${${L_variable}}")
  endforeach()
endfunction()

function(build_asm files outputs)
  foreach (source_file ${files})
    get_filename_component(filename ${source_file} NAME)
    set(output_file "${filename}.txt")
    add_custom_command(
      OUTPUT ${output_file}
      COMMAND ${CMAKE_ASM_MASM_COMPILER} /Zi /c /Cp /Fl /Fo "${output_file}" "${source_file}"
    )
    list(APPEND ${outputs} ${output_file})
  endforeach()
endfunction()

macro(get_WIN32_WINNT version)
    if (CMAKE_SYSTEM_VERSION)
        set(ver ${CMAKE_SYSTEM_VERSION})
        string(REGEX MATCH "^([0-9]+).([0-9])" ver ${ver})
        string(REGEX MATCH "^([0-9]+)" verMajor ${ver})
        # Check for Windows 10, b/c we'll need to convert to hex 'A'.
        if ("${verMajor}" MATCHES "10")
            set(verMajor "A")
            string(REGEX REPLACE "^([0-9]+)" ${verMajor} ver ${ver})
        endif ("${verMajor}" MATCHES "10")
        # Remove all remaining '.' characters.
        string(REPLACE "." "" ver ${ver})
        # Prepend each digit with a zero.
        string(REGEX REPLACE "([0-9A-Z])" "0\\1" ver ${ver})
        set(${version} "0x${ver}")
    endif(CMAKE_SYSTEM_VERSION)
endmacro(get_WIN32_WINNT)

