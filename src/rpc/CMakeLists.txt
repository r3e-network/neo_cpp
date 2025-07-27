# Neo RPC Module

# Create RPC library
add_library(neo_rpc OBJECT
    rpc_server_complete.cpp
    rpc_methods_complete.cpp
)

# Set include directories
target_include_directories(neo_rpc PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Link dependencies
target_link_libraries(neo_rpc PUBLIC
    neo_core
    neo_io
    neo_json
    neo_persistence
    neo_ledger
    neo_smartcontract
)

# Link httplib if available
if(httplib_FOUND)
    target_link_libraries(neo_rpc PUBLIC httplib::httplib)
    target_compile_definitions(neo_rpc PUBLIC NEO_HAS_HTTPLIB)
endif()