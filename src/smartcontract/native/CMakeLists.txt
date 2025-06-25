# Create the native contracts library
add_library(neo_native_contracts OBJECT
    native_contract.cpp
    native_contract_manager.cpp
    contract_management.cpp
    std_lib.cpp
    crypto_lib.cpp
    # crypto_lib_bls12_381.cpp # Temporarily disabled due to missing BLS12381Point type
    ledger_contract.cpp
    neo_token.cpp
    neo_token_account.cpp
    # neo_token_candidate.cpp # Temporarily disabled due to GetContractId issues
    # neo_token_committee.cpp # Temporarily disabled due to complex dependencies
    # neo_token_gas.cpp # Temporarily disabled due to complex dependencies
    neo_token_vote.cpp
    neo_token_transfer.cpp
    neo_token_persistence.cpp
    # gas_token.cpp # Temporarily disabled due to complex dependency issues
    policy_contract.cpp
    # oracle_contract.cpp # Temporarily disabled
    # oracle_contract_requests.cpp # Temporarily disabled
    # oracle_contract_handlers.cpp # Temporarily disabled
    # oracle_contract_persistence.cpp # Temporarily disabled
    # role_management.cpp # Temporarily disabled
    # name_service.cpp # Temporarily disabled  
    # name_service_records.cpp # Temporarily disabled
    # name_service_operations.cpp # Temporarily disabled
    # notary.cpp # Temporarily disabled
    # fungible_token.cpp # Temporarily disabled
    # non_fungible_token.cpp # Temporarily disabled
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
