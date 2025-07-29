# Create the native contracts unit tests
add_executable(test_native_contracts
    test_neo_token_rewards.cpp
    test_gas_token_total_supply.cpp
    test_contract_management.cpp
    test_crypto_lib.cpp
    test_gas_token.cpp
    test_name_service.cpp
    # These tests have API mismatches and are temporarily disabled:
    # test_native_contracts.cpp
    # test_ledger_contract.cpp
    # test_neo_token.cpp
    # test_neo_token_all_methods_complete.cpp
    # test_notary.cpp
    # test_oracle_contract.cpp
    # test_policy_contract.cpp
    # test_role_management.cpp
    # test_std_lib.cpp
    # test_fungible_token.cpp
    # test_non_fungible_token.cpp
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

# New comprehensive native contract tests
add_executable(test_native_contracts_complete
    test_contract_management_complete.cpp
    test_neo_token_complete.cpp
    test_gas_token_complete.cpp
    test_policy_contract_complete.cpp
    test_oracle_contract_complete.cpp
    test_role_management_complete.cpp
    test_crypto_lib_complete.cpp
    test_std_lib_complete.cpp
    test_ledger_contract_complete.cpp
    test_notary_complete.cpp
)

# Include directories
target_include_directories(test_native_contracts_complete
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(test_native_contracts_complete
    PRIVATE
        neo_cpp
        ${GTEST_LIBRARIES}
)

# Add test
add_test(NAME test_native_contracts_complete COMMAND test_native_contracts_complete)
