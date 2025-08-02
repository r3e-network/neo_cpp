# Create test executable
add_executable(plugins_tests
    # test_plugin_test.cpp # Disabled due to missing dependencies
    test_plugin_stub.cpp
)

# Link dependencies
target_link_libraries(plugins_tests
    PRIVATE
        neo_cpp
        GTest::gtest
        GTest::gtest_main
)

# Include directories
target_include_directories(plugins_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

# Add test
add_test(NAME plugins_tests COMMAND plugins_tests)
