# Security Test Suite CMake Configuration

# Create security test executable
add_executable(test_security
    test_security_suite.cpp
)

# Link required libraries
target_link_libraries(test_security
    PRIVATE
        neo_cryptography
        neo_network
        neo_ledger
        neo_wallets
        neo_vm
        neo_rpc
        neo_io
        neo_core
        neo_logging
        GTest::gtest
        GTest::gtest_main
        ${OPENSSL_LIBRARIES}
        ${Boost_LIBRARIES}
        Threads::Threads
)

# Add include directories
target_include_directories(test_security
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/third_party
)

# Set C++ standard
set_target_properties(test_security PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

# Add test to CTest
add_test(NAME SecurityTests COMMAND test_security)

# Set test properties
set_tests_properties(SecurityTests PROPERTIES
    TIMEOUT 300  # 5 minutes timeout for security tests
    LABELS "security;critical"
)

# Enable sanitizers for security testing if available
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(test_security PRIVATE
        -fsanitize=address
        -fsanitize=undefined
        -fno-omit-frame-pointer
    )
    target_link_options(test_security PRIVATE
        -fsanitize=address
        -fsanitize=undefined
    )
endif()

# Add coverage flags if enabled
if(ENABLE_COVERAGE)
    target_compile_options(test_security PRIVATE
        --coverage
        -fprofile-arcs
        -ftest-coverage
    )
    target_link_options(test_security PRIVATE
        --coverage
    )
endif()