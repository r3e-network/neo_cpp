cmake_minimum_required(VERSION 3.16)

# Find Google Test
find_package(GTest REQUIRED)

# Test sources
set(SDK_TEST_SOURCES
    test_main.cpp
    # test_core.cpp       # To be implemented
    # test_wallet.cpp     # To be implemented
    # test_transaction.cpp # To be implemented
    # test_rpc.cpp        # To be implemented
)

# Create test executable
add_executable(neo-sdk-tests ${SDK_TEST_SOURCES})

# Link with SDK and Google Test
target_link_libraries(neo-sdk-tests
    PRIVATE
        neo-sdk
        GTest::GTest
        GTest::Main
)

# Add tests
add_test(NAME neo-sdk-tests COMMAND neo-sdk-tests)

# Enable testing
enable_testing()