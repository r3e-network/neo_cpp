# Performance Regression Test Suite CMake Configuration

# Find Google Benchmark (optional but recommended)
find_package(benchmark QUIET)

# Create performance test executable
add_executable(test_performance
    test_performance_regression.cpp
)

# Link required libraries
target_link_libraries(test_performance
    PRIVATE
        neo_cryptography
        neo_network
        neo_ledger
        neo_vm
        neo_io
        neo_core
        neo_monitoring
        neo_logging
        GTest::gtest
        GTest::gtest_main
        ${OPENSSL_LIBRARIES}
        ${Boost_LIBRARIES}
        Threads::Threads
)

# Add Google Benchmark if available
if(benchmark_FOUND)
    target_link_libraries(test_performance PRIVATE benchmark::benchmark)
    target_compile_definitions(test_performance PRIVATE HAS_BENCHMARK=1)
else()
    message(STATUS "Google Benchmark not found - performance benchmarks will be limited")
    # Create stub benchmark namespace to allow compilation
    target_compile_definitions(test_performance PRIVATE NO_BENCHMARK=1)
endif()

# Add include directories
target_include_directories(test_performance
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/third_party
)

# Set C++ standard
set_target_properties(test_performance PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

# Add test to CTest
add_test(NAME PerformanceRegressionTests COMMAND test_performance)

# Set test properties
set_tests_properties(PerformanceRegressionTests PROPERTIES
    TIMEOUT 600  # 10 minutes timeout for performance tests
    LABELS "performance;regression"
)

# Enable optimizations for performance testing
target_compile_options(test_performance PRIVATE
    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:Release>:-march=native>
    $<$<CONFIG:Release>:-mtune=native>
)

# Add coverage flags if enabled
if(ENABLE_COVERAGE)
    target_compile_options(test_performance PRIVATE
        --coverage
        -fprofile-arcs
        -ftest-coverage
    )
    target_link_options(test_performance PRIVATE
        --coverage
    )
endif()

# Create custom target for running performance tests with benchmarks
if(benchmark_FOUND)
    add_custom_target(run_performance_benchmarks
        COMMAND test_performance --benchmark
        DEPENDS test_performance
        COMMENT "Running performance benchmarks..."
    )
endif()

# Create baseline update target
add_custom_target(update_performance_baseline
    COMMAND test_performance
    COMMAND ${CMAKE_COMMAND} -E copy_if_different 
            ${CMAKE_CURRENT_BINARY_DIR}/performance_baseline.json.new
            ${CMAKE_CURRENT_BINARY_DIR}/performance_baseline.json
    DEPENDS test_performance
    COMMENT "Updating performance baseline..."
)