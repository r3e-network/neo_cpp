cmake_minimum_required(VERSION 3.20)

# Keep only a single canonical node executable named "neo_node"
# Optionally include node subdirectory (builds core object library if present)
add_subdirectory(node)

# Build the modern CLI application (neo_cli_app)
add_subdirectory(cli)

# Helper to declare small launcher-style node binaries
function(neo_add_node_target target source_file)
  add_executable(${target} ${source_file})
  target_link_libraries(${target} PRIVATE neo_cpp neo_node_core)
  target_include_directories(${target} PRIVATE ${CMAKE_SOURCE_DIR}/include)
  install(TARGETS ${target} RUNTIME DESTINATION bin)
endfunction()

# Single node target
neo_add_node_target(neo_node ${CMAKE_CURRENT_SOURCE_DIR}/neo_node.cpp)

# Optionally build CLI application (disabled by default; legacy neo-cli target incomplete)
option(NEO_BUILD_CLI "Build neo-cli application" OFF)
if(NEO_BUILD_CLI)
  add_subdirectory(neo-cli)
endif()

# Additional node entrypoints mirroring the C# presets. Enabled by default so
# release packaging picks them up.
option(NEO_BUILD_EXTRA_NODES "Build auxiliary node launchers (testnet, production variants, mini)" ON)
if(NEO_BUILD_EXTRA_NODES)
  neo_add_node_target(neo_testnet_node ${CMAKE_SOURCE_DIR}/neo_testnet_node.cpp)
  neo_add_node_target(neo_testnet_production ${CMAKE_SOURCE_DIR}/neo_testnet_production.cpp)
  neo_add_node_target(mini_neo_node ${CMAKE_SOURCE_DIR}/mini_neo_node.cpp)
endif()

# Compatibility alias used by older scripts (same binary as neo_node)
option(NEO_BUILD_WORKING_NODE_ALIAS "Build compatibility alias working_neo_node" ON)
if(NEO_BUILD_WORKING_NODE_ALIAS)
  neo_add_node_target(working_neo_node ${CMAKE_CURRENT_SOURCE_DIR}/neo_node.cpp)
endif()
