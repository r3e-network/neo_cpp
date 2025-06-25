# Create the RPC server plugin
add_library(neo_rpcserver MODULE
    rpc_server.cpp
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
        ${BOOST_LIBRARIES}
)

# Install targets
install(TARGETS neo_rpcserver
    LIBRARY DESTINATION lib/neo/plugins
)
