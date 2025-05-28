# Create the CLI library
add_library(neo_cli OBJECT
    cli.cpp
    main_service.cpp
    main_service_blockchain.cpp
    main_service_node.cpp
    main_service_wallet.cpp
    base_commands.cpp
    blockchain_commands.cpp
    node_commands.cpp
    wallet_commands.cpp
    wallet_service_commands.cpp
    transaction_commands.cpp
    contract_commands.cpp
    type_converters.cpp
)

# Include directories
target_include_directories(neo_cli
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(neo_cli
    PUBLIC
        neo_node
        neo_rpc
        neo_wallets
        neo_smartcontract
        neo_cryptography
        nlohmann_json::nlohmann_json
)
