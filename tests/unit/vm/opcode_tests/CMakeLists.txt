# VM opcode parity tests
add_executable(test_vm_opcodes_parity
    test_arithmetic_complete.cpp
    test_array_complete.cpp
    test_control_complete.cpp
    test_stack_complete.cpp
)

target_include_directories(test_vm_opcodes_parity
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(test_vm_opcodes_parity
    PRIVATE
        neo_vm
        neo_io
        neo_cryptography
        gtest
        gtest_main
)

add_test(NAME vm_opcodes_parity COMMAND test_vm_opcodes_parity)
set_tests_properties(vm_opcodes_parity PROPERTIES TIMEOUT 60)
