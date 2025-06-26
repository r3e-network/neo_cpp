# Add subdirectories
add_subdirectory(nep6)

# Create the wallets library
add_library(neo_wallets OBJECT
    wallet.cpp
    wallet_account.cpp
    wallet_factory.cpp
    asset_descriptor.cpp
    wallet_transaction.cpp
    verification_contract.cpp
    wallet_manager.cpp
    helper.cpp
    key_pair.cpp
    transfer_output.cpp
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
        neo_smartcontract
        nlohmann_json::nlohmann_json
)
