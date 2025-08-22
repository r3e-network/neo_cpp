# Consensus Test Module CMakeLists.txt

set(CONSENSUS_TEST_SOURCES
    test_consensus_basic.cpp
    test_consensus_extended.cpp
    test_consensus_message.cpp
    test_consensus_context.cpp
    test_consensus_service.cpp
    test_primary_index_calculation.cpp
    test_byzantine_fault_tolerance.cpp
    test_view_change_recovery.cpp
    test_witness_assembly.cpp
)

# Main consensus test executable
add_executable(test_consensus ${CONSENSUS_TEST_SOURCES})

target_link_libraries(test_consensus
    PRIVATE
        neo_consensus
        neo_ledger
        neo_network
        neo_cryptography
        neo_wallets
        gtest
        gtest_main
)

target_include_directories(test_consensus
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/tests/include
)

# Individual comprehensive test executable
add_executable(test_consensus_complete test_consensus_complete.cpp)
target_link_libraries(test_consensus_complete
    PRIVATE
        neo_consensus
        neo_ledger
        neo_network
        neo_cryptography
        gtest
        gtest_main
)

# Add tests to CTest
add_test(NAME consensus_basic COMMAND test_consensus)
add_test(NAME consensus_complete COMMAND test_consensus_complete)

# Set test properties
set_tests_properties(consensus_basic PROPERTIES TIMEOUT 30)
set_tests_properties(consensus_complete PROPERTIES TIMEOUT 60)
