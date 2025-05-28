# Create the integration tests
add_executable(test_integration
    test_blockchain.cpp
    test_network.cpp
    test_rpc.cpp
    test_smartcontract.cpp
)

# Include directories
target_include_directories(test_integration
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(test_integration
    PRIVATE
        neo_cpp
        ${GTEST_LIBRARIES}
)

# Add test
add_test(NAME test_integration COMMAND test_integration)
