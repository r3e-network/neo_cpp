# VM Test Module CMakeLists.txt

set(VM_TEST_SOURCES
    test_debugger.cpp
    test_script_builder.cpp
    test_stack_item.cpp
    test_execution_engine.cpp
)

# Main VM test executable
add_executable(test_vm ${VM_TEST_SOURCES})

target_link_libraries(test_vm
    PRIVATE
        neo_cpp
        gtest
        gtest_main
)

target_include_directories(test_vm
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/tests/include
)

# Individual test executable for comprehensive opcode tests
option(NEO_ENABLE_VM_OPCODE_PARITY_TESTS "Enable comprehensive VM opcode parity tests" OFF)
if(NEO_ENABLE_VM_OPCODE_PARITY_TESTS)
    add_executable(test_vm_opcodes_complete test_vm_opcodes_complete.cpp)
    target_link_libraries(test_vm_opcodes_complete
        PRIVATE
            neo_vm
            neo_io
            gtest
            gtest_main
    )
    add_test(NAME vm_opcodes_complete COMMAND test_vm_opcodes_complete)
    set_tests_properties(vm_opcodes_complete PROPERTIES TIMEOUT 60)
else()
    message(STATUS "Skipping VM opcode parity tests (NEO_ENABLE_VM_OPCODE_PARITY_TESTS=OFF)")
endif()

# Add tests to CTest
add_test(NAME vm_basic COMMAND test_vm)

# Set test properties
set_tests_properties(vm_basic PROPERTIES TIMEOUT 30)
