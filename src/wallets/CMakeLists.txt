# Add subdirectories
add_subdirectory(nep6)

# Create the wallets library
add_library(neo_wallets OBJECT
    stubs.cpp  # Temporary stubs for linking
    # wallet.cpp # Disabled - needs JsonWriter fixes
    # wallet_account.cpp # Disabled - needs JsonWriter fixes
    # wallet_factory.cpp # Disabled - needs JsonWriter fixes
    # asset_descriptor.cpp # Disabled - needs JsonWriter fixes
    # wallet_transaction.cpp # Disabled - needs JsonWriter fixes
    # verification_contract.cpp # Disabled - needs JsonWriter fixes
    # wallet_manager.cpp # Disabled - needs JsonWriter fixes
    helper.cpp
    # key_pair.cpp # Disabled - needs JsonWriter fixes
    # transfer_output.cpp # Disabled - needs JsonWriter fixes
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
