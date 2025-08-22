# Performance Test Module CMakeLists.txt
# Performance benchmarks disabled - benchmark library not available

# Create minimal performance test using GTest instead
add_executable(test_performance
    test_performance_simple.cpp
)

target_link_libraries(test_performance
    PRIVATE
        neo_cryptography
        neo_vm
        neo_io
        gtest
        gtest_main
)

target_include_directories(test_performance
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

# Add to CTest
add_test(NAME test_performance COMMAND test_performance)