cmake_minimum_required(VERSION 3.20)

# Keep only a single canonical node executable named "neo_node"

# Optionally include node subdirectory (builds core object library if present)
add_subdirectory(node)

# Build the modern CLI application (neo_cli_app)
add_subdirectory(cli)

# Single node target
add_executable(neo_node neo_node.cpp)
target_link_libraries(neo_node PRIVATE neo_cpp neo_node_core)
target_include_directories(neo_node PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Install only the canonical node
install(TARGETS neo_node RUNTIME DESTINATION bin)

# Optionally build CLI application (disabled by default; legacy neo-cli target incomplete)
option(NEO_BUILD_CLI "Build neo-cli application" OFF)
if(NEO_BUILD_CLI)
  add_subdirectory(neo-cli)
endif()
