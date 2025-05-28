# Create the node application
add_executable(neo_node_app main.cpp)

# Include directories
target_include_directories(neo_node_app
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

# Link dependencies
target_link_libraries(neo_node_app
    PRIVATE
        neo_cpp
)

# Install targets
install(TARGETS neo_node_app
    RUNTIME DESTINATION bin
)
