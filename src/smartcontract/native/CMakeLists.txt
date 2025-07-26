# Create the native contracts library - SIMPLIFIED FOR WORKING BUILD
add_library(neo_native_contracts OBJECT
    native_contract.cpp
    native_contract_manager.cpp
    contract_management.cpp
    std_lib.cpp
    crypto_lib.cpp
    ledger_contract.cpp
    neo_token.cpp
    neo_token_account.cpp
    neo_token_vote.cpp
    neo_token_transfer.cpp
    neo_token_persistence.cpp
    neo_token_gas.cpp
    neo_token_candidate.cpp
    neo_token_committee.cpp
    policy_contract.cpp
    # Temporarily disable complex contracts to avoid circular dependencies
    # gas_token.cpp # Disabled - complex dependencies
    # oracle_contract.cpp # Disabled - complex dependencies
    # oracle_contract_handlers.cpp # Disabled - complex dependencies
    # role_management.cpp # Disabled - complex dependencies
    # fungible_token.cpp # Disabled - complex dependencies
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
