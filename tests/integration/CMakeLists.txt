# Integration Test Module CMakeLists.txt

# Main integration test executable
add_executable(test_integration
    test_stub.cpp
)

target_link_libraries(test_integration
    PRIVATE
        neo_cpp
        gtest
        gtest_main
)

target_include_directories(test_integration
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/tests/include
)

# Add tests to CTest
add_test(NAME integration_basic COMMAND test_integration)

# Set test properties (longer timeouts for integration tests)
set_tests_properties(integration_basic PROPERTIES TIMEOUT 120)
