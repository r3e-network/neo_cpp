# Add subdirectories
add_subdirectory(statistics)

# Create the plugins library
add_library(neo_plugins STATIC
    plugin_manager.cpp
    plugin_base.cpp
    # rpc_server_plugin.cpp  # Disabled - RpcServer constructor mismatch
    # rpc_plugin.cpp  # Keep disabled due to missing dependencies
    # application_logs_plugin.cpp  # Disabled - API mismatches
    # state_service_plugin.cpp  # Disabled - API mismatches
    # dbft_plugin.cpp  # Disabled - API mismatches
    # plugin_registration_impl.cpp  # Disabled - conflicts with existing PluginFactory class
    # plugin_registration.cpp  # Replaced with new implementation
    # statistics_plugin.cpp  # Keep disabled for now
    # test_plugin.cpp  # Keep disabled for now
)

# Include directories
target_include_directories(neo_plugins
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(neo_plugins
    PUBLIC
        neo_io
        neo_node_core
        neo_rpc
        neo_consensus
        neo_wallets
        neo_logging
        neo_core
)
