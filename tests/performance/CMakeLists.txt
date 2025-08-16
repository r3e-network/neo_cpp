# Performance Test Module CMakeLists.txt

set(PERFORMANCE_TEST_SOURCES
    benchmark_crypto.cpp
    benchmark_vm_execution.cpp
    benchmark_transaction_processing.cpp
    benchmark_block_validation.cpp
    benchmark_consensus.cpp
    benchmark_network_throughput.cpp
    benchmark_state_access.cpp
    benchmark_serialization.cpp
    benchmark_memory_usage.cpp
    benchmark_database_operations.cpp
)

# Main performance benchmark executable
add_executable(test_performance ${PERFORMANCE_TEST_SOURCES})

target_link_libraries(test_performance
    PRIVATE
        neo_ledger
        neo_consensus
        neo_network
        neo_smartcontract
        neo_vm
        neo_cryptography
        neo_io
        benchmark::benchmark
        benchmark::benchmark_main
        GTest::GTest
)

target_include_directories(test_performance
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/tests/include
)

# Individual benchmark executables
add_executable(benchmark_crypto_only benchmark_crypto.cpp)
target_link_libraries(benchmark_crypto_only
    PRIVATE
        neo_cryptography
        neo_io
        benchmark::benchmark
        benchmark::benchmark_main
)

add_executable(benchmark_vm_only benchmark_vm_execution.cpp)
target_link_libraries(benchmark_vm_only
    PRIVATE
        neo_vm
        neo_io
        benchmark::benchmark
        benchmark::benchmark_main
)

# Add tests to CTest (performance tests have longer timeouts)
add_test(NAME performance_full COMMAND test_performance --benchmark_min_time=1)
add_test(NAME performance_crypto COMMAND benchmark_crypto_only --benchmark_min_time=1)
add_test(NAME performance_vm COMMAND benchmark_vm_only --benchmark_min_time=1)

# Set test properties (longer timeouts for performance tests)
set_tests_properties(performance_full PROPERTIES TIMEOUT 300)
set_tests_properties(performance_crypto PROPERTIES TIMEOUT 120)
set_tests_properties(performance_vm PROPERTIES TIMEOUT 120)