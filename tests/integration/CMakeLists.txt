# Integration Test Module CMakeLists.txt

set(INTEGRATION_TEST_SOURCES
    test_blockchain_persistence.cpp
    test_end_to_end_scenarios.cpp
    test_consensus_integration.cpp
    # test_network_sync.cpp  # Missing file
    # test_smart_contract_deployment.cpp  # Missing file
    # test_token_transfers.cpp  # Missing file
    # test_multi_node_consensus.cpp  # Missing file
    # test_state_synchronization.cpp  # Missing file
    # test_oracle_integration.cpp  # Missing file
    # test_full_node_validation.cpp  # Missing file
)

# Main integration test executable
add_executable(test_integration ${INTEGRATION_TEST_SOURCES})

target_link_libraries(test_integration
    PRIVATE
        neo_ledger
        neo_consensus
        neo_network
        neo_smartcontract
        neo_vm
        neo_cryptography
        neo_wallets
        neo_io
        gtest
        gtest_main
)

target_include_directories(test_integration
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/tests/include
)

# Individual end-to-end test executable
add_executable(test_e2e_scenarios test_end_to_end_scenarios.cpp)
target_link_libraries(test_e2e_scenarios
    PRIVATE
        neo_ledger
        neo_consensus
        neo_network
        neo_smartcontract
        neo_vm
        neo_cryptography
        neo_wallets
        neo_io
        gtest
        gtest_main
)

# Add tests to CTest
add_test(NAME integration_basic COMMAND test_integration)
add_test(NAME integration_e2e COMMAND test_e2e_scenarios)

# Set test properties (longer timeouts for integration tests)
set_tests_properties(integration_basic PROPERTIES TIMEOUT 120)
set_tests_properties(integration_e2e PROPERTIES TIMEOUT 180)