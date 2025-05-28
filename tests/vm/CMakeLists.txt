# Add GoogleTest
find_package(GTest REQUIRED)

# Add test executable
add_executable(vm_tests
    execution_context_test.cpp
    stack_item_tests.cpp
    reference_counter_tests.cpp
    # Add other test files here
)

# Link with GoogleTest and our VM library
target_link_libraries(vm_tests
    PRIVATE
        neo_vm
        GTest::GTest
        GTest::Main
)

# Add test
add_test(NAME vm_tests COMMAND vm_tests)

# Make sure tests are built with the same flags as the main project
target_compile_features(vm_tests PRIVATE cxx_std_17)

# Copy test data if needed
# file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_data/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/test_data/)
