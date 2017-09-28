

function(append_group_sources LIST)
  cmake_parse_arguments(ARG "" "FILTER;ROOT" "DIRECTORIES;EXCLUDES" ${ARGN})

  if(ARG_EXCLUDES)
    foreach(L_cexclude ${ARG_EXCLUDES})
      get_filename_component(L_cexclude ${L_cexclude} ABSOLUTE)
      list(APPEND L_EXCLUDES_ABS ${L_cexclude})
    endforeach()
  endif()

  if(ARG_ROOT)
    get_filename_component(ARG_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/${ARG_ROOT}" ABSOLUTE)
  else()
    set(ARG_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
  endif()

  string(REGEX MATCHALL "[^\|]+" subfilters ${ARG_FILTER})
  foreach(cname ${ARG_DIRECTORIES})

    get_filename_component(cdir "${ARG_ROOT}/${cname}" ABSOLUTE)
    
    set(cgroup ${cname})
    string(REGEX REPLACE "\\.\\./" "" cgroup ${cname})
    string(REGEX REPLACE "\\./" "Sources/" cgroup ${cgroup})
    string(REGEX REPLACE "/" "\\\\" cgroup ${cgroup})
      
    foreach(cfilter ${subfilters})
      #message(STATUS "glob: ${cgroup} | ${cname} ${cfilter} | ${cdir}")
      file(GLOB founds "${cdir}/${cfilter}")
      list(REMOVE_ITEM founds "" ${L_EXCLUDES_ABS})
      list(APPEND founds_files ${founds})
      source_group(${cgroup} FILES ${founds})
    endforeach()
  endforeach()
  set(${LIST} ${${LIST}} ${founds_files} PARENT_SCOPE)
endfunction()
