# Add subdirectories
add_subdirectory(cryptography)
add_subdirectory(io)
add_subdirectory(json)
add_subdirectory(logging)
add_subdirectory(extensions)
add_subdirectory(console_service)
add_subdirectory(persistence)
add_subdirectory(network)
add_subdirectory(ledger)
add_subdirectory(vm)
add_subdirectory(smartcontract)
add_subdirectory(wallets)
add_subdirectory(plugins)
add_subdirectory(consensus)
add_subdirectory(rpc)
add_subdirectory(cli)

# Create the main library
add_library(neo_cpp
    $<TARGET_OBJECTS:neo_cryptography>
    $<TARGET_OBJECTS:neo_mpttrie>
    $<TARGET_OBJECTS:neo_io>
    $<TARGET_OBJECTS:neo_json>
    $<TARGET_OBJECTS:neo_logging>
    $<TARGET_OBJECTS:neo_extensions>
    $<TARGET_OBJECTS:neo_console_service>
    $<TARGET_OBJECTS:neo_persistence>
    $<TARGET_OBJECTS:neo_network>
    $<TARGET_OBJECTS:neo_ledger>
    $<TARGET_OBJECTS:neo_vm>
    $<TARGET_OBJECTS:neo_smartcontract>
    $<TARGET_OBJECTS:neo_wallets>
    $<TARGET_OBJECTS:neo_wallets_nep6>
    $<TARGET_OBJECTS:neo_plugins>
    $<TARGET_OBJECTS:neo_consensus>
    $<TARGET_OBJECTS:neo_rpc>
    $<TARGET_OBJECTS:neo_cli>
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
install(TARGETS neo_cpp
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
