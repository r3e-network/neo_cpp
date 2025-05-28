# Create the VM unit tests
add_executable(test_vm
    test_stack_item.cpp
    test_script.cpp
    test_execution_engine.cpp
    test_vm_json.cpp
    test_lte_gte.cpp
    vm_json_test_base.cpp
    test_debugger.cpp
    test_evaluation_stack.cpp
    test_execution_context_state.cpp
    test_struct.cpp
    test_slot.cpp
    test_try_catch.cpp
    test_reference_counter.cpp
    test_script_builder.cpp
    test_script_converter.cpp
)

# Include directories
target_include_directories(test_vm
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(test_vm
    PRIVATE
        neo_cpp
        ${GTEST_LIBRARIES}
)

# Copy test data files to the build directory
add_custom_command(TARGET test_vm POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/Tests
    ${CMAKE_CURRENT_BINARY_DIR}/Tests
    COMMENT "Copying test data files to build directory"
)

# Add test
add_test(NAME test_vm COMMAND test_vm)
