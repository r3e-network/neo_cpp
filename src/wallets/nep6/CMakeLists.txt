# Create the NEP6 wallet library
add_library(neo_wallets_nep6 OBJECT
    nep6_wallet.cpp
    scrypt_parameters.cpp
    nep6_wallet_factory.cpp
)

# Include directories
target_include_directories(neo_wallets_nep6
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(neo_wallets_nep6
    PUBLIC
        neo_wallets
        neo_cryptography
        neo_io
        neo_smartcontract
        nlohmann_json::nlohmann_json
)
