# Add subdirectories
add_subdirectory(native)

# Create the smartcontract unit tests
add_executable(test_smartcontract
    test_contract.cpp
    test_application_engine.cpp
    test_contract_state.cpp
    test_transaction_verifier.cpp
    test_nef_file.cpp
    test_contract_parameters_context.cpp
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
