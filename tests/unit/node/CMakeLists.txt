# Create the node unit tests
add_executable(test_node
    test_node.cpp
    test_consensus.cpp
    test_neo_system.cpp
)

# Include directories
target_include_directories(test_node
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(test_node
    PRIVATE
        neo_cpp
        ${GTEST_LIBRARIES}
)

# Add test
add_test(NAME test_node COMMAND test_node)
