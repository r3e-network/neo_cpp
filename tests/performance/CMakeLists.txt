# Performance Regression Tests
add_executable(test_performance
    test_performance_regression.cpp
)

target_link_libraries(test_performance
    neo_core
    neo_ledger
    neo_io
    GTest::gtest_main
)

add_test(NAME PerformanceRegressionTests COMMAND test_performance)
set_tests_properties(PerformanceRegressionTests PROPERTIES 
    LABELS "performance;regression"
    TIMEOUT 60
)
