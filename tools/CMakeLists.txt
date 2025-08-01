cmake_minimum_required(VERSION 3.15)
project(neo_tools VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add tool executables
add_executable(neo_cli_tool cli/main.cpp)
target_link_libraries(neo_cli_tool PRIVATE 
    neo_core 
    neo_persistence 
    neo_ledger 
    neo_blockchain
    neo_rpc 
    neo_smartcontract
    neo_native_contracts
    neo_network
    neo_node
    neo_wallets
    # neo_wallets_nep6  # Disabled - has too many dependencies
    neo_cryptography
    neo_io
    neo_logging
    neo_json
    neo_extensions
    neo_vm
)

add_executable(neo_gui gui/main.cpp)
target_link_libraries(neo_gui PRIVATE 
    neo_core 
    neo_persistence 
    neo_ledger 
    neo_blockchain
    neo_rpc 
    neo_smartcontract
    neo_native_contracts
    neo_network
    neo_node
    neo_wallets
    neo_cryptography
    neo_io
    neo_logging
    neo_json
    neo_extensions
    neo_vm
)

# RPC server test tool
add_executable(test_rpc_server test_rpc_server.cpp)
target_link_libraries(test_rpc_server PRIVATE 
    neo_rpc 
    neo_core 
    neo_io 
    neo_json 
    neo_logging
    neo_persistence 
    neo_ledger 
    neo_blockchain
    neo_smartcontract
    neo_native_contracts
    neo_network
    neo_node
    neo_cryptography
    neo_extensions
    neo_vm
    neo_wallets
)

# Install
install(TARGETS
    neo_cli_tool
    neo_gui
    test_rpc_server
    RUNTIME DESTINATION bin
)
