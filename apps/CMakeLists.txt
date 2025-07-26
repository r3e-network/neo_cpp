# Add the node subdirectory
add_subdirectory(node)

# Add the CLI subdirectory
add_subdirectory(cli)

# Create simple neo node application
add_executable(simple_neo_node simple_neo_node.cpp)
target_link_libraries(simple_neo_node PRIVATE neo_cpp)
target_include_directories(simple_neo_node PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Create working neo node application  
add_executable(working_neo_node working_neo_node.cpp)
target_link_libraries(working_neo_node PRIVATE neo_cpp)
target_include_directories(working_neo_node PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Create complete neo node application with full mainnet/testnet support
add_executable(neo_node_complete neo_node_complete.cpp)
target_link_libraries(neo_node_complete PRIVATE neo_cpp nlohmann_json::nlohmann_json)
target_include_directories(neo_node_complete PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Create basic neo node application
add_executable(neo_node neo_node.cpp)
target_link_libraries(neo_node PRIVATE neo_cpp)
target_include_directories(neo_node PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Create minimal neo node application
add_executable(neo_node_minimal neo_node_minimal.cpp)
target_link_libraries(neo_node_minimal PRIVATE neo_cpp)
target_include_directories(neo_node_minimal PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Create core demo application - demonstrates Neo C++ core functionality
add_executable(core_demo core_demo.cpp)
target_link_libraries(core_demo PRIVATE neo_cpp)
target_include_directories(core_demo PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Install targets for node executables
install(TARGETS simple_neo_node working_neo_node
    RUNTIME DESTINATION bin
)
