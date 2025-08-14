# Benchmarks CMakeLists.txt

# Find Google Benchmark
find_package(benchmark)

if(benchmark_FOUND)
    message(STATUS "Google Benchmark found, building benchmarks")
    
    # Main benchmark suite
    add_executable(benchmark_suite
        benchmark_suite.cpp
    )
    
    target_link_libraries(benchmark_suite
        PRIVATE
            benchmark::benchmark
            neo_core
            neo_network
            neo_ledger
            neo_cryptography
            neo_smartcontract
            neo_consensus
            neo_monitoring
            neo_profiling
            neo_vm
            ${CMAKE_THREAD_LIBS_INIT}
    )
    
    target_compile_features(benchmark_suite PRIVATE cxx_std_20)
    
    # Micro benchmarks
    add_executable(micro_benchmarks
        micro_benchmarks.cpp
    )
    
    target_link_libraries(micro_benchmarks
        PRIVATE
            benchmark::benchmark
            neo_core
            neo_cryptography
            ${CMAKE_THREAD_LIBS_INIT}
    )
    
    # Custom benchmark runner with comparison
    add_executable(benchmark_runner
        benchmark_runner.cpp
    )
    
    target_link_libraries(benchmark_runner
        PRIVATE
            benchmark::benchmark
            neo_core
            ${CMAKE_THREAD_LIBS_INIT}
    )
    
    # Add benchmark target
    add_custom_target(run_benchmarks
        COMMAND benchmark_suite --benchmark_out=benchmark_results.json --benchmark_out_format=json
        COMMAND benchmark_suite --benchmark_out=benchmark_results.csv --benchmark_out_format=csv
        DEPENDS benchmark_suite
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Running performance benchmarks..."
    )
    
    # Add regression benchmark target
    add_custom_target(benchmark_regression
        COMMAND benchmark_suite --benchmark_filter=.* --benchmark_repetitions=10 --benchmark_report_aggregates_only=true
        DEPENDS benchmark_suite
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Running benchmark regression tests..."
    )
    
else()
    message(STATUS "Google Benchmark not found, skipping benchmark build")
    message(STATUS "Install Google Benchmark: https://github.com/google/benchmark")
endif()