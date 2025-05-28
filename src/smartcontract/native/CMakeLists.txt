# Create the native contracts library
add_library(neo_native_contracts OBJECT
    native_contract.cpp
    native_contract_manager.cpp
    contract_management.cpp
    std_lib.cpp
    crypto_lib.cpp
    crypto_lib_bls12_381.cpp
    ledger_contract.cpp
    neo_token.cpp
    neo_token_account.cpp
    neo_token_candidate.cpp
    neo_token_committee.cpp
    neo_token_gas.cpp
    neo_token_vote.cpp
    neo_token_transfer.cpp
    neo_token_persistence.cpp
    gas_token.cpp
    policy_contract.cpp
    oracle_contract.cpp
    oracle_contract_requests.cpp
    oracle_contract_handlers.cpp
    oracle_contract_persistence.cpp
    role_management.cpp
    name_service.cpp
    name_service_records.cpp
    name_service_operations.cpp
    notary.cpp
    fungible_token.cpp
    non_fungible_token.cpp
    hash_index_state.cpp
    id_list.cpp
    node_list.cpp
    neo_account_state.cpp
    oracle_request.cpp
)

# Include directories
target_include_directories(neo_native_contracts
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(neo_native_contracts
    PUBLIC
        neo_io
        neo_persistence
        neo_cryptography
        neo_vm
)
