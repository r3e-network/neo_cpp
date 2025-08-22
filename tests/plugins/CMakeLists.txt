# Create test executable
add_executable(plugins_tests
    test_plugin_test.cpp  # Use existing file
    # test_plugin_stub.cpp  # Missing file
)

# Link dependencies
target_link_libraries(plugins_tests
    PRIVATE
        neo_cpp
        gtest
        gtest_main
)

# Include directories
target_include_directories(plugins_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

# Add test
add_test(NAME plugins_tests COMMAND plugins_tests)
