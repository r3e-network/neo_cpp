# Consensus Test Module CMakeLists.txt

set(CONSENSUS_TEST_SOURCES
    test_consensus_message.cpp
)

# Main consensus test executable
add_executable(test_consensus ${CONSENSUS_TEST_SOURCES})

target_sources(test_consensus
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/consensus/consensus_message.cpp
        ${CMAKE_SOURCE_DIR}/src/consensus/consensus_state.cpp
        ${CMAKE_SOURCE_DIR}/src/consensus/recovery_message.cpp
        logger_stub.cpp
        common_logger_stub.cpp
        fixed8_stub.cpp
        extensions_stub.cpp
        neo3_transaction_stub.cpp
        ledger_crypto_stubs.cpp
)

target_link_libraries(test_consensus
    PRIVATE
        neo_io
        neo_logging
        gtest
        gtest_main
)

target_include_directories(test_consensus
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/tests/include
)

# Individual comprehensive test executable
add_test(NAME consensus_basic COMMAND test_consensus)
set_tests_properties(consensus_basic PROPERTIES TIMEOUT 30)
