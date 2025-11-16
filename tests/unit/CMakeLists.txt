# Add subdirectories
add_subdirectory(cryptography)
add_subdirectory(io)
add_subdirectory(json)
add_subdirectory(extensions)
add_subdirectory(console_service)
add_subdirectory(persistence)
add_subdirectory(network)
add_subdirectory(settings)
add_subdirectory(ledger)
add_subdirectory(vm)
add_subdirectory(wallets)
add_subdirectory(rpc)
option(NEO_ENABLE_EXTENDED_UNIT_TESTS "Enable extended unit test modules" OFF)
option(NEO_ENABLE_SMARTCONTRACT_PARITY_TESTS "Enable smart contract parity tests" OFF)
option(NEO_ENABLE_VM_OPCODE_PARITY_TESTS "Enable VM opcode parity tests" OFF)
if(NEO_ENABLE_EXTENDED_UNIT_TESTS OR NEO_ENABLE_SMARTCONTRACT_PARITY_TESTS)
    add_subdirectory(smartcontract)
    add_subdirectory(plugins)
    add_subdirectory(consensus)
    add_subdirectory(cli)
    add_subdirectory(monitoring)
else()
    message(STATUS "Skipping extended unit test modules (NEO_ENABLE_EXTENDED_UNIT_TESTS=OFF)")
endif()

if(NEO_ENABLE_VM_OPCODE_PARITY_TESTS)
    add_subdirectory(vm/opcode_tests)
else()
    message(STATUS "Skipping VM opcode parity tests (NEO_ENABLE_VM_OPCODE_PARITY_TESTS=OFF)")
endif()

# Main unit tests
# add_executable(test_main
#     test_protocol_settings_all_methods_complete.cpp
# )

# target_include_directories(test_main
#     PRIVATE
#         ${CMAKE_SOURCE_DIR}/include
#         ${CMAKE_CURRENT_SOURCE_DIR}
# )

# target_link_libraries(test_main
#     PRIVATE
#         neo_cpp
#         ${GTEST_LIBRARIES}
# )

# add_test(NAME test_main COMMAND test_main)
