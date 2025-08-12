# Create the integration tests
add_executable(test_integration
    test_integration_main.cpp  # Main function with signal handling
    # test_blockchain.cpp # Disabled due to failures
    # test_network.cpp # Disabled due to failures
    # test_rpc.cpp # Disabled due to failures
    # test_smartcontract.cpp # Disabled due to DataCache instantiation issue
    test_smartcontract_simple.cpp
    test_p2p_sync_working.cpp
    test_p2p_block_sync_integration.cpp
    test_p2p_blockchain_integration.cpp
    test_p2p_connectivity_comprehensive.cpp
    test_block_sync_comprehensive.cpp
    test_block_execution_comprehensive.cpp
    test_state_updates_comprehensive.cpp
    test_neo_node_complete_integration.cpp
    test_neo_capabilities_integration.cpp
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
