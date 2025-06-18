# Node component CMakeLists.txt

# Collect node source files
set(NODE_SOURCES
    neo_node.cpp
    neo_node_initialization.cpp
    neo_node_processing.cpp
)

# Create node library
add_library(neo-node STATIC ${NODE_SOURCES})

# Link dependencies
target_link_libraries(neo-node
    PUBLIC
        neo-core
)

# Set target properties
set_target_properties(neo-node PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
)

# Main executable
add_executable(neo-node-main main.cpp)
target_link_libraries(neo-node-main PRIVATE neo-node)

# Set executable properties
set_target_properties(neo-node-main PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    OUTPUT_NAME neo-node
) 