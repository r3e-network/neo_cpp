# Create the plugins unit tests
add_executable(test_plugins
    test_plugins.cpp
    test_rpc_server_plugin.cpp
    test_application_logs_plugin.cpp
    test_state_service_plugin.cpp
    test_dbft_plugin.cpp
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
