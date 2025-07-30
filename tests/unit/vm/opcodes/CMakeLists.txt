# Create the simple VM opcode tests
add_executable(test_vm_opcodes_simple
    test_opcodes_arithmetic_simple.cpp
)

# Create the VM opcode tests (now enabled with JSON test data)
add_executable(test_vm_opcodes
    test_opcodes_arithmetic.cpp
    test_opcodes_arrays.cpp
    test_opcodes_bitwiselogic.cpp
    test_opcodes_control.cpp
    test_opcodes_push.cpp
    test_opcodes_slot.cpp
    test_opcodes_splice.cpp
    test_opcodes_stack.cpp
    test_opcodes_types.cpp
)

# Include directories for simple tests
target_include_directories(test_vm_opcodes_simple
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies for simple tests
target_link_libraries(test_vm_opcodes_simple
    PRIVATE
        neo_cpp
        ${GTEST_LIBRARIES}
)

# Add test
add_test(NAME test_vm_opcodes_simple COMMAND test_vm_opcodes_simple)

# Include directories for full tests
target_include_directories(test_vm_opcodes
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies for full tests
target_link_libraries(test_vm_opcodes
    PRIVATE
        neo_cpp
        ${GTEST_LIBRARIES}
        nlohmann_json::nlohmann_json
)

# Copy test data files to the build directory
add_custom_command(TARGET test_vm_opcodes POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/tests/unit/vm/Tests
    ${CMAKE_CURRENT_BINARY_DIR}/Tests
    COMMENT "Copying VM test data files to build directory"
)

# Add test
add_test(NAME test_vm_opcodes COMMAND test_vm_opcodes)