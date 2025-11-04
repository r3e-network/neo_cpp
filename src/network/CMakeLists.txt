# Create the network library
add_library(neo_network OBJECT
    payload_factory.cpp  # Real payload factory implementation
    network_address.cpp  # NetworkAddress implementation
    ip_endpoint.cpp
    p2p/ip_endpoint.cpp  # P2P version of IPEndPoint
    p2p/message_impl.cpp  # Fixed message implementation
    # p2p/message_complete.cpp  # Replaced with fixed version
    # p2p/message_simple.cpp  # Replaced with complete implementation
    tcp_server.cpp
    tcp_client.cpp  # Add TcpClient implementation
    tcp_connection.cpp  # TcpConnection implementation
    p2p_server.cpp # P2P server implementation
    peer_discovery_service.cpp  # Add PeerDiscoveryService implementation
    upnp.cpp
    p2p/node_capability.cpp
    p2p/network_address.cpp
    p2p/inventory_vector.cpp
    p2p/connection.cpp
    p2p/tcp_connection.cpp
    p2p/remote_node.cpp
    p2p/local_node.cpp
    p2p/local_node_handlers.cpp  # New implementation with complete message handling
    # p2p/local_node_connection_impl.cpp  # Removed - API mismatch
    # p2p/local_node_messaging.cpp # Temporarily disabled
    p2p/peer.cpp
    p2p/peer_list.cpp
    p2p/task_manager.cpp # Task manager for P2P network tasks
    # p2p/transaction_router.cpp # Disabled - needs major refactoring
    p2p/channels_config.cpp
    p2p/payloads/extensible_payload.cpp
    p2p/payloads/merkle_block_payload.cpp
    p2p/payloads/oracle_response.cpp
    p2p/payloads/addr_payload.cpp
    p2p/payloads/inv_payload.cpp
    p2p/payloads/ping_payload.cpp
    p2p/payloads/headers_payload.cpp
    p2p/payloads/get_blocks_payload.cpp
    p2p/payloads/get_block_by_index_payload.cpp
    p2p/payloads/version_payload.cpp
    p2p/payloads/verack_payload.cpp
    p2p/payloads/transaction_payload.cpp
    p2p/payloads/block_payload.cpp
    p2p/payloads/filter_load_payload.cpp
    p2p/payloads/filter_add_payload.cpp
    p2p/payloads/filter_clear_payload.cpp
    p2p/payloads/get_addr_payload.cpp
    p2p/payloads/get_headers_payload.cpp
    p2p/payloads/mempool_payload.cpp
    p2p/payloads/not_found_payload.cpp
    p2p/payloads/reject_payload.cpp
    p2p/payloads/neo3_transaction.cpp
    p2p/payloads/conflicts.cpp
    p2p/payloads/high_priority.cpp
    p2p/payloads/not_valid_before.cpp
    p2p/payloads/header.cpp
    p2p/payloads/network_address_with_time.cpp
    p2p/protocol_handler.cpp
    p2p/block_sync_manager.cpp
    connection_pool.cpp
)

# Include directories
target_include_directories(neo_network
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(neo_network
    PUBLIC
        neo_core
        neo_io
        neo_ledger
        neo_persistence
        neo_logging
        ZLIB::ZLIB
)

if(Boost_FOUND)
    target_link_libraries(neo_network PUBLIC Boost::boost)
    # Try to link specific components if available
    if(TARGET Boost::system)
        target_link_libraries(neo_network PUBLIC Boost::system)
    endif()
    if(TARGET Boost::thread)
        target_link_libraries(neo_network PUBLIC Boost::thread)
    endif()
    target_compile_definitions(neo_network PUBLIC NEO_HAS_BOOST)
else()
    message(WARNING "Boost not found - network features may be limited")
endif()
