# Add subdirectories
add_subdirectory(statistics)

# Create the plugins library
add_library(neo_plugins STATIC
    plugin_manager.cpp
    plugin_base.cpp
    # rpc_server_plugin.cpp  # Temporarily disabled due to template issues
    # rpc_plugin.cpp  # Keep disabled due to missing dependencies
    # application_logs_plugin.cpp  # Temporarily disabled due to template issues
    # state_service_plugin.cpp  # Temporarily disabled due to template issues
    # dbft_plugin.cpp  # Temporarily disabled due to template issues
    plugin_registration.cpp
    # statistics_plugin.cpp  # Temporarily disabled due to template issues
    # test_plugin.cpp  # Temporarily disabled due to template issues
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
