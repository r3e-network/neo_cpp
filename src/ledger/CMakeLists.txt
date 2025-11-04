# Neo Ledger Module

# Create ledger library with complete implementations
add_library(neo_ledger OBJECT
    blockchain_utilities.cpp  # Utility implementations for blockchain
    # blockchain_fallback.cpp  # Disabled - conflicts with blockchain_complete.cpp
    ledger_dummy.cpp
    blockchain_complete.cpp  # Complete blockchain implementation
    block.cpp
    block_header.cpp
    header_cache.cpp
    memory_pool_enhanced.cpp
    event_system.cpp
    pool_item.cpp
    transaction_attribute.cpp
    transaction_output.cpp
    coin_reference.cpp
    witness.cpp
    signer.cpp
    witness_rule.cpp
    transaction_pool_manager.cpp  # Transaction pool management
    state_synchronization_manager.cpp  # State synchronization
    transaction_validator.cpp
    transaction_verification_context.cpp
    neo_system.cpp
)

# Set include directories
target_include_directories(neo_ledger PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Link dependencies
target_link_libraries(neo_ledger PUBLIC
    neo_core
    neo_cryptography
    neo_io
)
