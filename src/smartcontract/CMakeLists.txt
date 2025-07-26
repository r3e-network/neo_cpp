# Add subdirectories
add_subdirectory(native)

# Create the smartcontract library
add_library(neo_smartcontract OBJECT
    contract.cpp
    contract_state.cpp
    application_engine.cpp
    application_engine_core.cpp
    application_engine_system_calls.cpp
    system_calls_runtime.cpp
    system_calls_storage.cpp
    system_calls_contract.cpp
    system_calls_crypto.cpp
    system_calls_json.cpp
    storage_iterator.cpp
    transaction_verifier.cpp
    nef_file.cpp
    method_token.cpp
    # contract_parameters_context.cpp # Temporarily disabled due to complex type conversion issues
)

# Include directories
target_include_directories(neo_smartcontract
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(neo_smartcontract
    PUBLIC
        neo_io
        neo_vm
        neo_cryptography
        neo_persistence
        neo_ledger
        neo_logging
        neo_core
        neo_native_contracts
)
