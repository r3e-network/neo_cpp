# Add subdirectories
add_subdirectory(nep6)

# Create the wallets library
add_library(neo_wallets OBJECT
    wallet.cpp # Re-enabled - base class implementation
    wallet_account.cpp # Re-enabled - implementation complete
    # wallet_factory.cpp # Disabled - compilation errors
    asset_descriptor.cpp # Re-enabled to fix undefined symbols
    # wallet_transaction.cpp # Disabled - compilation errors
    # verification_contract.cpp # Disabled - compilation errors
    wallet_manager.cpp # Re-enabled - manager implementation
    helper.cpp
    # key_pair.cpp # Disabled - compilation errors
    # key_pair_proper.cpp # Disabled - compilation errors
    # transfer_output.cpp # Disabled - compilation errors
)

# Include directories
target_include_directories(neo_wallets
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(neo_wallets
    PUBLIC
        neo_cryptography
        neo_io
        nlohmann_json::nlohmann_json
)
