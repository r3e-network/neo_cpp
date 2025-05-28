# Create the RPC library
add_library(neo_rpc OBJECT
    rpc_request.cpp
    rpc_response.cpp
    rpc_server.cpp
    rpc_methods.cpp
    rpc_client.cpp
)

# Include directories
target_include_directories(neo_rpc
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(neo_rpc
    PUBLIC
        neo_node
        neo_smartcontract
        neo_cryptography
        nlohmann_json::nlohmann_json
)
