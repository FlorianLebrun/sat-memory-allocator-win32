set(target boost)

# Detect toolset to use for Boost
string(SUBSTRING ${MSVC_TOOLSET_VERSION} 0 2 msvc_major)
string(SUBSTRING ${MSVC_TOOLSET_VERSION} 2 1 msvc_minor)
set(toolset "msvc-${msvc_major}.${msvc_minor}")

# Detect address mode to use for Boost
getTargetCPUAddressMode(address_model)

# Compile b2 tool
set(BOOST_b2 "${CMAKE_CURRENT_BINARY_DIR}/b2.exe")
add_custom_command(OUTPUT ${BOOST_b2}
  COMMENT "Boost unpacking..."
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  COMMAND ${CMAKE_COMMAND} -E tar x "${CMAKE_CURRENT_SOURCE_DIR}/boost_1_70_0.7z"
  COMMAND call bootstrap.bat
)

# Build Boost libraries
message( STATUS "Architecture: ${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}" )
set(BOOST_log_file "${CMAKE_CURRENT_BINARY_DIR}/build-log.txt")
add_custom_command(OUTPUT ${BOOST_log_file} DEPENDS ${BOOST_b2}
  COMMENT "Boost building..."
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  COMMAND call "${BOOST_b2}"
      "--stagedir=${CMAKE_CURRENT_BINARY_DIR}/release"
      --build-type=minimal
      --with-log
      -j8
      toolset=${toolset}
      address-model=${address_model}
      threading=multi
      link=static
      runtime-link=shared
      variant=release
      stage
  COMMAND call "${BOOST_b2}"
      "--stagedir=${CMAKE_CURRENT_BINARY_DIR}/debug"
      --build-type=minimal
      --with-log
      -j8
      toolset=${toolset}
      address-model=${address_model}
      threading=multi
      link=static
      runtime-link=shared
      variant=debug
      stage
  COMMAND echo "done" > ${BOOST_log_file}
)

# Create library
add_library(${target} INTERFACE ${BOOST_log_file})
set_target_properties(${target} PROPERTIES FOLDER "ThirdParty")
target_include_directories(${target} INTERFACE "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(${target} INTERFACE debug "${CMAKE_CURRENT_BINARY_DIR}/debug/lib/*.lib")
target_link_libraries(${target} INTERFACE optimized "${CMAKE_CURRENT_BINARY_DIR}/release/lib/*.lib")
