# Add subdirectories
add_subdirectory(statistics)

# Create the plugins library
add_library(neo_plugins OBJECT
    plugin_manager.cpp
    plugin_base.cpp
    rpc_server_plugin.cpp
    application_logs_plugin.cpp
    state_service_plugin.cpp
    dbft_plugin.cpp
    plugin_registration.cpp
    test_plugin.cpp
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
        neo_node
        neo_rpc
        neo_consensus
        neo_wallets
)
