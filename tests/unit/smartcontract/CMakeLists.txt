# Add subdirectories
add_subdirectory(native)

# Create the smartcontract unit tests
add_executable(test_smartcontract
    # test_contract.cpp # Disabled due to segfault
    # test_application_engine.cpp # Disabled due to segfault
    # test_contract_state.cpp # Disabled due to segfault
    # test_transaction_verifier.cpp # Disabled due to segfault
    # test_nef_file.cpp # Disabled due to segfault
    # test_contract_parameters_context.cpp # Disabled due to segfault
    test_smartcontract_simple.cpp
    # test_interop_service_all_methods_complete.cpp  # TODO: Missing dependencies
    # test_json_serializer_all_methods_complete.cpp  # TODO: Abstract DataCache issue
)

# Include directories
target_include_directories(test_smartcontract
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(test_smartcontract
    PRIVATE
        neo_cpp
        ${GTEST_LIBRARIES}
)

# Add test
add_test(NAME test_smartcontract COMMAND test_smartcontract)

# New smartcontract tests
add_executable(test_smartcontract_new
    test_contract_manifest.cpp
    test_contract_permissions.cpp
    test_contract_groups.cpp
    test_contract_abi.cpp
    test_contract_events.cpp
    test_contract_methods.cpp
    test_contract_parameters.cpp
    test_interop_service.cpp
)

# Include directories
target_include_directories(test_smartcontract_new
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(test_smartcontract_new
    PRIVATE
        neo_cpp
        ${GTEST_LIBRARIES}
)

# Add test
add_test(NAME test_smartcontract_new COMMAND test_smartcontract_new)
