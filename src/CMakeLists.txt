# Add subdirectories (core modules only)
add_subdirectory(core)
add_subdirectory(cryptography)
add_subdirectory(io)
add_subdirectory(json)
add_subdirectory(logging)
add_subdirectory(extensions)
# add_subdirectory(console_service) # Temporarily disabled
add_subdirectory(console_service) # Re-enabled after fixing compilation issues
# add_subdirectory(console_service) # Re-disabled due to missing dependencies
add_subdirectory(persistence)
add_subdirectory(network) # Re-enabling to fix compilation errors
add_subdirectory(ledger)
add_subdirectory(blockchain)
add_subdirectory(vm)
add_subdirectory(smartcontract)
add_subdirectory(wallets)
add_subdirectory(plugins) # Re-enabled after fixing compilation errors
add_subdirectory(consensus)
add_subdirectory(rpc) # Re-enabled with JSON API fixes
add_subdirectory(node) # Add node directory for neo::node::NeoSystem
# add_subdirectory(cli) # Disabled due to API mismatches

target_include_directories(neo_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(neo_core PUBLIC
    nlohmann_json::nlohmann_json
)

# Create the main library (core modules only)
add_library(neo_cpp
    $<TARGET_OBJECTS:neo_core>
    $<TARGET_OBJECTS:neo_cryptography>
    $<TARGET_OBJECTS:neo_mpttrie>
    $<TARGET_OBJECTS:neo_io>
    $<TARGET_OBJECTS:neo_json>
    $<TARGET_OBJECTS:neo_logging>
    $<TARGET_OBJECTS:neo_extensions>
    # $<TARGET_OBJECTS:neo_console_service> # Re-disabled due to missing dependencies
    $<TARGET_OBJECTS:neo_persistence>
    $<TARGET_OBJECTS:neo_network> # Re-enabled to fix issues
    $<TARGET_OBJECTS:neo_ledger>
    $<TARGET_OBJECTS:neo_blockchain>
    $<TARGET_OBJECTS:neo_vm>
    $<TARGET_OBJECTS:neo_smartcontract>
    $<TARGET_OBJECTS:neo_native_contracts>
    $<TARGET_OBJECTS:neo_wallets>
    $<TARGET_OBJECTS:neo_wallets_nep6> # Re-enabled - required by tests
    $<TARGET_OBJECTS:neo_plugins> # Re-enabled after fixing compilation errors
    $<TARGET_OBJECTS:neo_logging> # Ensure logging is included
    # $<TARGET_OBJECTS:neo_statistics_plugin> # Disabled due to compilation errors
    $<TARGET_OBJECTS:neo_consensus>
    $<TARGET_OBJECTS:neo_node> # Add node library for neo::node::NeoSystem
    $<TARGET_OBJECTS:neo_rpc> # Re-enabled with JSON API fixes
    # $<TARGET_OBJECTS:neo_cli> # Disabled due to API mismatches
)

# Link dependencies
target_link_libraries(neo_cpp
    PUBLIC
        OpenSSL::SSL
        OpenSSL::Crypto
        nlohmann_json::nlohmann_json
        spdlog::spdlog
)

# Link RocksDB if found
if(RocksDB_FOUND)
    target_link_libraries(neo_cpp PUBLIC RocksDB::rocksdb)
    target_compile_definitions(neo_cpp PUBLIC NEO_HAS_ROCKSDB)
endif()

# Link Boost if found
if(Boost_FOUND)
    target_link_libraries(neo_cpp PUBLIC
        Boost::system
        Boost::filesystem
        Boost::thread
    )
    target_compile_definitions(neo_cpp PUBLIC NEO_HAS_BOOST)
endif()

# Install targets
install(TARGETS neo_cpp neo_core
    EXPORT neo_cpp-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(EXPORT neo_cpp-targets
    FILE neo_cpp-targets.cmake
    NAMESPACE neo::
    DESTINATION lib/cmake/neo_cpp
)
