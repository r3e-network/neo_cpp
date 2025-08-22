# Ledger Test Module CMakeLists.txt

set(LEDGER_TEST_SOURCES
    test_transaction_safe.cpp
    test_transaction_complete.cpp
    test_block.cpp
    test_blockchain_complete.cpp
    test_ledger_simple.cpp
    test_ledger_extended.cpp
    test_header_cache.cpp
    test_blockchain_cache_simple.cpp
    test_memory_pool_all_methods_complete.cpp  # Use existing file
    # test_state_machine.cpp  # Missing file
)

# Main ledger test executable
add_executable(test_ledger ${LEDGER_TEST_SOURCES})

target_link_libraries(test_ledger
    PRIVATE
        neo_ledger
        neo_io
        neo_cryptography
        neo_smartcontract
        gtest
        gtest_main
)

target_include_directories(test_ledger
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/tests/include
)

# Individual comprehensive test executables
add_executable(test_transaction_complete test_transaction_complete.cpp)
target_link_libraries(test_transaction_complete
    PRIVATE
        neo_ledger
        neo_io
        neo_cryptography
        gtest
        gtest_main
)

add_executable(test_blockchain_complete test_blockchain_complete.cpp)
target_link_libraries(test_blockchain_complete
    PRIVATE
        neo_ledger
        neo_io
        neo_cryptography
        neo_smartcontract
        gtest
        gtest_main
)

# Add tests to CTest
add_test(NAME ledger_basic COMMAND test_ledger)
add_test(NAME ledger_transaction_complete COMMAND test_transaction_complete)
add_test(NAME ledger_blockchain_complete COMMAND test_blockchain_complete)

# Set test properties
set_tests_properties(ledger_basic PROPERTIES TIMEOUT 30)
set_tests_properties(ledger_transaction_complete PROPERTIES TIMEOUT 60)
set_tests_properties(ledger_blockchain_complete PROPERTIES TIMEOUT 60)
