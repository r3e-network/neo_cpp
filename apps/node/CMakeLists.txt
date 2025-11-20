# Optional legacy/compatibility node executable. The primary supported entrypoint
# is apps/neo_node.cpp; enable this target only if downstream tooling expects
# the historical binary name.
option(NEO_BUILD_COMPAT_NODE "Build legacy node entrypoint (compatibility shim)" OFF)
if(NEO_BUILD_COMPAT_NODE)
  add_executable(neo_node_exec main.cpp)
  target_include_directories(neo_node_exec
    PRIVATE
      ${CMAKE_SOURCE_DIR}/include
  )
  target_link_libraries(neo_node_exec
    PRIVATE
      neo_cpp
      neo_node_core
  )
  install(TARGETS neo_node_exec
    RUNTIME DESTINATION bin
  )
endif()
