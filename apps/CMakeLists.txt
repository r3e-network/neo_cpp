# Add subdirectories for applications
add_subdirectory(node)
add_subdirectory(cli)

# Main Neo C++ node application
add_executable(neo_node neo_node.cpp)
target_link_libraries(neo_node PRIVATE 
    neo_cpp
)

# Minimal Neo C++ node application
add_executable(neo_node_minimal neo_node_minimal.cpp)
target_link_libraries(neo_node_minimal PRIVATE 
    neo_cpp
)
