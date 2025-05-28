# Create the consensus unit tests
add_executable(test_consensus
    test_consensus_message.cpp
    test_consensus_context.cpp
    test_consensus_service.cpp
)

# Include directories
target_include_directories(test_consensus
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(test_consensus
    PRIVATE
        neo_cpp
        ${GTEST_LIBRARIES}
)

# Add test
add_test(NAME test_consensus COMMAND test_consensus)
