# Create the native contracts unit tests
add_executable(test_native_contracts
    test_native_contracts.cpp
    test_name_service.cpp
    test_contract_management.cpp
    test_crypto_lib.cpp
    test_gas_token.cpp
    test_ledger_contract.cpp
    test_neo_token.cpp
    test_notary.cpp
    test_oracle_contract.cpp
    test_policy_contract.cpp
    test_role_management.cpp
    test_std_lib.cpp
    test_fungible_token.cpp
    test_non_fungible_token.cpp
)

# Include directories
target_include_directories(test_native_contracts
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(test_native_contracts
    PRIVATE
        neo_cpp
        ${GTEST_LIBRARIES}
)

# Add test
add_test(NAME test_native_contracts COMMAND test_native_contracts)
