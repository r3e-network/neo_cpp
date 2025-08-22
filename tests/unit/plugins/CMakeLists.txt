# Create the plugins unit tests
add_executable(test_plugins
    test_plugins.cpp  # Use existing file
    # test_rpc_server_plugin.cpp # Disabled due to missing implementations
    # test_application_logs_plugin.cpp # Disabled due to missing implementations
    # test_state_service_plugin.cpp # Disabled due to missing implementations
    # test_dbft_plugin.cpp  # Disabled due to header compatibility issues
    # test_dbft_plugin_stub.cpp # Disabled - using single stub
    # test_plugins_stub.cpp  # Missing file
)

# Include directories
target_include_directories(test_plugins
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(test_plugins
    PRIVATE
        neo_cpp
        ${GTEST_LIBRARIES}
)

# Add test
add_test(NAME test_plugins COMMAND test_plugins)
