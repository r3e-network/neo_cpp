# Node component CMakeLists.txt

# Collect node source files (enable real NeoSystem)
set(NODE_SOURCES
    neo_system.cpp
    # neo_node.cpp
    # neo_node_initialization.cpp
    # neo_node_processing.cpp
)

# Create node core object library
add_library(neo_node_core OBJECT ${NODE_SOURCES})

# Link dependencies
target_link_libraries(neo_node_core
    PUBLIC
        neo_core
        neo_ledger
        neo_network
        neo_persistence
)

# Set target properties
set_target_properties(neo_node_core PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
)

# Include directories
target_include_directories(neo_node_core
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

# Main executable - disabled for now
# add_executable(neo-node-main main.cpp)
# target_link_libraries(neo-node-main PRIVATE neo-node)
# 
# # Set executable properties
# set_target_properties(neo-node-main PROPERTIES
#     CXX_STANDARD 20
#     CXX_STANDARD_REQUIRED ON
#     CXX_EXTENSIONS OFF
#     OUTPUT_NAME neo-node
# ) 
