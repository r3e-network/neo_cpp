# Ledger Test Module CMakeLists.txt

option(NEO_ENABLE_LEGACY_LEDGER_TESTS "Enable legacy aggregated ledger parity tests" OFF)
if(NEO_ENABLE_LEGACY_LEDGER_TESTS)
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
            ${CMAKE_SOURCE_DIR}/tests
    )

    add_test(NAME ledger_basic COMMAND test_ledger)
    set_tests_properties(ledger_basic PROPERTIES TIMEOUT 30)
else()
    message(STATUS "Skipping legacy ledger parity aggregate tests (NEO_ENABLE_LEGACY_LEDGER_TESTS=OFF)")
endif()

# Individual comprehensive test executables
add_executable(test_transaction_complete test_transaction_complete.cpp)
target_link_libraries(test_transaction_complete
    PRIVATE
        neo_ledger
        neo_io
        neo_cryptography
        neo_network
        neo_wallets
        neo_cpp
        gtest
        gtest_main
)

add_executable(test_blockchain_complete test_blockchain_complete.cpp)
target_link_libraries(test_blockchain_complete
    PRIVATE
        neo_cpp
        gtest
        gtest_main
)
target_include_directories(test_blockchain_complete
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/tests
)

add_executable(test_signer_json test_signer_json.cpp)
target_link_libraries(test_signer_json
    PRIVATE
        neo_cpp
        gtest
        gtest_main
)
target_include_directories(test_signer_json
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
)

# Add tests to CTest
add_test(NAME ledger_transaction_complete COMMAND test_transaction_complete)
add_test(NAME ledger_blockchain_complete COMMAND test_blockchain_complete)
add_test(NAME ledger_signer_json COMMAND test_signer_json)

# Set test properties
set_tests_properties(ledger_transaction_complete PROPERTIES TIMEOUT 60)
set_tests_properties(ledger_blockchain_complete PROPERTIES TIMEOUT 60)
set_tests_properties(ledger_signer_json PROPERTIES TIMEOUT 30)
