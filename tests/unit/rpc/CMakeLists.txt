# Create the RPC unit tests
add_executable(test_rpc
    test_rpc_request.cpp
    test_rpc_response.cpp
    test_rpc_methods.cpp
    test_rpc_client.cpp
)

# Include directories
target_include_directories(test_rpc
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(test_rpc
    PRIVATE
        neo_cpp
        ${GTEST_LIBRARIES}
)

# Add test
add_test(NAME test_rpc COMMAND test_rpc)
