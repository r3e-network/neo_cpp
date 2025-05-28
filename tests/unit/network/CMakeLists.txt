# Create the Network unit tests
add_executable(test_network
    test_ip_endpoint.cpp
    test_message.cpp
    test_payloads.cpp
    test_tcp_server.cpp
    p2p/test_message.cpp
    p2p/test_payloads.cpp
    p2p/test_connection.cpp
    p2p/test_peer.cpp
)

# Include directories
target_include_directories(test_network
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(test_network
    PRIVATE
        neo_cpp
        ${GTEST_LIBRARIES}
)

# Add test
add_test(NAME test_network COMMAND test_network)
