# Create the Monitoring unit tests
add_executable(test_monitoring
    # test_metrics.cpp  # Disabled - MetricsCollector not fully implemented
    test_performance_monitor.cpp  # Performance monitor tests
)

# Include directories
target_include_directories(test_monitoring
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(test_monitoring
    PRIVATE
        neo_cpp
        GTest::gtest
        GTest::gtest_main
)

# Add test
add_test(NAME test_monitoring COMMAND test_monitoring)