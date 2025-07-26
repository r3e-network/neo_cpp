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
# add_subdirectory(network) # Temporarily disabled due to compilation issues
add_subdirectory(ledger)
add_subdirectory(vm)
add_subdirectory(smartcontract)
# add_subdirectory(wallets) # Temporarily disabled due to missing crypto implementations
# add_subdirectory(plugins) # Temporarily disabled
add_subdirectory(consensus)
add_subdirectory(rpc) # Re-enabled with JSON API fixes
# add_subdirectory(cli) # Temporarily disabled due to compilation issues

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
    # $<TARGET_OBJECTS:neo_network> # Temporarily disabled due to compilation issues
    $<TARGET_OBJECTS:neo_ledger>
    $<TARGET_OBJECTS:neo_vm>
    $<TARGET_OBJECTS:neo_smartcontract>
    $<TARGET_OBJECTS:neo_native_contracts>
    # $<TARGET_OBJECTS:neo_wallets> # Temporarily disabled
    # $<TARGET_OBJECTS:neo_wallets_nep6> # Temporarily disabled
    # $<TARGET_OBJECTS:neo_plugins> # Temporarily disabled
    $<TARGET_OBJECTS:neo_consensus>
    $<TARGET_OBJECTS:neo_rpc> # Re-enabled with JSON API fixes
    # $<TARGET_OBJECTS:neo_cli> # Temporarily disabled due to compilation issues
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
