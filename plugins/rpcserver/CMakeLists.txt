# Create the RPC server plugin
add_library(neo_rpcserver MODULE
    rpc_server.cpp
    rpc_server_plugin.cpp
    rpc_handler.cpp
    rpc_error.cpp
    rpc_request.cpp
    rpc_response.cpp
    http_server.cpp
    blockchain_rpc_handler.cpp
    node_rpc_handler.cpp
    smartcontract_rpc_handler.cpp
    wallet_rpc_handler.cpp
    utility_rpc_handler.cpp
)

# Include directories
target_include_directories(neo_rpcserver
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(neo_rpcserver
    PRIVATE
        neo_cpp
        Boost::system
        Boost::thread
)

# Install targets
install(TARGETS neo_rpcserver
    LIBRARY DESTINATION lib/neo/plugins
)
