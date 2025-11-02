# SmartContract Test Module CMakeLists.txt

set(SMARTCONTRACT_TEST_SOURCES
    test_smartcontract_extended.cpp
    test_application_engine.cpp
    test_contract_manifest.cpp
    test_contract_state.cpp
    test_interop_service.cpp
    test_contract.cpp
    test_native_contract.cpp
    test_storage.cpp
)

# Main smartcontract test executable
add_executable(test_smartcontract ${SMARTCONTRACT_TEST_SOURCES})

target_link_libraries(test_smartcontract
    PRIVATE
        neo_smartcontract
        neo_vm
        neo_ledger
        neo_io
        neo_cryptography
        gtest
        gtest_main
)

target_include_directories(test_smartcontract
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/tests/include
)

# Individual comprehensive test executables
add_executable(test_native_contracts_complete test_native_contracts_complete.cpp)
target_link_libraries(test_native_contracts_complete
    PRIVATE
        neo_smartcontract
        neo_vm
        neo_ledger
        neo_io
        gtest
        gtest_main
)

add_executable(test_nep_standards_complete test_nep_standards_complete.cpp)
target_link_libraries(test_nep_standards_complete
    PRIVATE
        neo_smartcontract
        neo_vm
        neo_ledger
        neo_io
        gtest
        gtest_main
)

# Add tests to CTest
add_test(NAME smartcontract_basic COMMAND test_smartcontract)
add_test(NAME smartcontract_native_complete COMMAND test_native_contracts_complete)
add_test(NAME smartcontract_nep_complete COMMAND test_nep_standards_complete)

# Set test properties
set_tests_properties(smartcontract_basic PROPERTIES TIMEOUT 30)
set_tests_properties(smartcontract_native_complete PROPERTIES TIMEOUT 60)
set_tests_properties(smartcontract_nep_complete PROPERTIES TIMEOUT 60)

neo_add_test(test_application_engine_stack test_application_engine_stack.cpp)
