# Neo RPC Module

# Create RPC library
add_library(neo_rpc OBJECT
    rpc_server.cpp                      # Main RPC server implementation
    rpc_methods_complete.cpp            # Re-enabled full RPC methods
    # rpc_server_complete_production.cpp
    # metrics_endpoint.cpp
    # rpc_server_unified.cpp
    # rpc_server_impl.cpp
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
    neo_monitoring
)

# Link httplib if available (support both package and header-only detection)
if(HTTPLIB_FOUND)
    # Header-only usage; target may not exist
    target_compile_definitions(neo_rpc PUBLIC NEO_HAS_HTTPLIB)
elseif(httplib_FOUND)
    # Found via find_package with target
    target_link_libraries(neo_rpc PUBLIC httplib::httplib)
    target_compile_definitions(neo_rpc PUBLIC NEO_HAS_HTTPLIB)
endif()
