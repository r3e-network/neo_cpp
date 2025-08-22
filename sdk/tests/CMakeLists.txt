cmake_minimum_required(VERSION 3.16)

# Find Google Test (optional)
find_package(GTest QUIET)
if(NOT GTest_FOUND)
    message(WARNING "GTest not found - SDK tests will be skipped")
    return()
endif()

# Test sources
set(SDK_TEST_SOURCES
    test_main.cpp
    test_core_types.cpp       # Core types tests
    test_wallet.cpp           # Wallet functionality tests
    test_rpc_client.cpp       # RPC client tests
    test_transaction_manager.cpp # Transaction manager tests
    test_nep17_token.cpp      # NEP17 token tests
)

# Create test executable
add_executable(neo-sdk-tests ${SDK_TEST_SOURCES})

# Include directories
target_include_directories(neo-sdk-tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

# Link with SDK and Google Test
target_link_libraries(neo-sdk-tests
    PRIVATE
        neo-sdk
        GTest::GTest
        GTest::Main
        pthread  # For threading support in tests
)

# Set test properties
set_target_properties(neo-sdk-tests PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/tests
)

# Add tests with categories
add_test(NAME neo-sdk-tests COMMAND neo-sdk-tests)
add_test(NAME neo-sdk-core-types COMMAND neo-sdk-tests --gtest_filter=CoreTypesTest.*)
add_test(NAME neo-sdk-wallet COMMAND neo-sdk-tests --gtest_filter=WalletTest.*)
add_test(NAME neo-sdk-rpc COMMAND neo-sdk-tests --gtest_filter=RpcClientTest.*)
add_test(NAME neo-sdk-transaction COMMAND neo-sdk-tests --gtest_filter=TransactionManagerTest.*)
add_test(NAME neo-sdk-nep17 COMMAND neo-sdk-tests --gtest_filter=NEP17TokenTest.*)

# Enable testing with verbose output
enable_testing()

# Add custom target to run tests
add_custom_target(run-sdk-tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS neo-sdk-tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running SDK unit tests..."
)

# Coverage target (optional, requires gcov/lcov)
if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND CMAKE_COMPILER_IS_GNUCXX)
    target_compile_options(neo-sdk-tests PRIVATE --coverage)
    target_link_options(neo-sdk-tests PRIVATE --coverage)
    
    add_custom_target(sdk-coverage
        COMMAND lcov --capture --directory . --output-file coverage.info
        COMMAND lcov --remove coverage.info '/usr/*' --output-file coverage.info
        COMMAND lcov --list coverage.info
        DEPENDS run-sdk-tests
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating SDK test coverage report..."
    )
endif()