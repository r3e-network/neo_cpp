# Add the node subdirectory
add_subdirectory(node)

# Add the CLI subdirectory
# add_subdirectory(cli)  # Commented out due to linker errors

# Add the neo-cli subdirectory
# add_subdirectory(neo-cli)  # Commented out due to linker errors

# Create production neo node application
add_executable(neo_node_app neo_node.cpp)
target_link_libraries(neo_node_app PRIVATE neo_cpp)
target_include_directories(neo_node_app PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Create production neo node application
# add_executable(neo_node_production neo_node_production.cpp)
# target_link_libraries(neo_node_production PRIVATE neo_cpp)
# target_include_directories(neo_node_production PRIVATE ${CMAKE_SOURCE_DIR}/include)

# # Create working neo node application  
# add_executable(working_neo_node working_neo_node.cpp)
# target_link_libraries(working_neo_node PRIVATE neo_cpp)
# target_include_directories(working_neo_node PRIVATE ${CMAKE_SOURCE_DIR}/include)

# # Create BLS12-381 test application
# add_executable(test_bls12_381 test_bls12_381.cpp)
# target_link_libraries(test_bls12_381 PRIVATE neo_cpp)
# target_include_directories(test_bls12_381 PRIVATE ${CMAKE_SOURCE_DIR}/include)

# # Create testnet node
# add_executable(testnet_node testnet_node.cpp)
# target_link_libraries(testnet_node PRIVATE neo_logging neo_core neo_rpc neo_persistence neo_cryptography neo_io neo_extensions)
# target_include_directories(testnet_node PRIVATE ${CMAKE_SOURCE_DIR}/include)

# # Create complete testnet node
# add_executable(neo_testnet_complete neo_testnet_complete.cpp)
# target_link_libraries(neo_testnet_complete PRIVATE neo_cpp)
# target_include_directories(neo_testnet_complete PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Commented out demo/test executables - files were cleaned up
# To re-enable, restore the corresponding source files

# # Create standalone testnet node (minimal dependencies)
# add_executable(neo_testnet_standalone neo_testnet_standalone.cpp)
# target_link_libraries(neo_testnet_standalone PRIVATE neo_cryptography neo_io neo_extensions neo_core neo_logging neo_persistence neo_ledger neo_blockchain neo_smartcontract neo_native_contracts neo_network neo_node neo_vm neo_wallets neo_json)
# target_include_directories(neo_testnet_standalone PRIVATE ${CMAKE_SOURCE_DIR}/include)

# # Create crypto demo (minimal dependencies)
# add_executable(neo_crypto_demo neo_crypto_demo.cpp)
# target_link_libraries(neo_crypto_demo PRIVATE neo_cpp)
# target_include_directories(neo_crypto_demo PRIVATE ${CMAKE_SOURCE_DIR}/include)

# # Create BLS demo (truly minimal dependencies)
# add_executable(bls_demo bls_demo.cpp)
# target_link_libraries(bls_demo PRIVATE neo_cpp)
# target_include_directories(bls_demo PRIVATE ${CMAKE_SOURCE_DIR}/include)

# # Create minimal neo node (reduced dependencies)
# add_executable(minimal_neo_node minimal_neo_node_fixed.cpp)
# target_link_libraries(minimal_neo_node PRIVATE neo_cryptography neo_io neo_extensions neo_core neo_logging neo_persistence neo_ledger neo_blockchain neo_network neo_wallets neo_smartcontract neo_vm neo_native_contracts)
# target_include_directories(minimal_neo_node PRIVATE ${CMAKE_SOURCE_DIR}/include)

# # Create simple test node application
# add_executable(test_simple_node test_simple_node.cpp)
# target_link_libraries(test_simple_node PRIVATE neo_cpp)
# target_include_directories(test_simple_node PRIVATE ${CMAKE_SOURCE_DIR}/include)

# # Create testnet node application
# add_executable(neo_testnet_node neo_testnet_node.cpp)
# target_link_libraries(neo_testnet_node PRIVATE neo_cpp)
# target_include_directories(neo_testnet_node PRIVATE ${CMAKE_SOURCE_DIR}/include)

# # Create fixed node application (no threading issues)
# add_executable(neo_node_fixed neo_node_fixed.cpp)
# target_link_libraries(neo_node_fixed PRIVATE neo_cpp)
# target_include_directories(neo_node_fixed PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Create functional neo node
add_executable(neo_functional_node neo_functional_node.cpp)
target_link_libraries(neo_functional_node PRIVATE neo_cpp)
target_include_directories(neo_functional_node PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Create safe neo node (no network features to avoid crashes)
add_executable(neo_node_safe neo_node_safe.cpp)
target_link_libraries(neo_node_safe PRIVATE neo_cpp)
target_include_directories(neo_node_safe PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Create production neo node
add_executable(neo_node_production neo_node_production.cpp)
target_link_libraries(neo_node_production PRIVATE neo_cpp)
target_include_directories(neo_node_production PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Create production ready neo node
add_executable(neo_node_production_ready neo_node_production_ready.cpp)
target_link_libraries(neo_node_production_ready PRIVATE neo_cpp)
target_include_directories(neo_node_production_ready PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Create full neo node
add_executable(neo_node_full neo_node_full.cpp)
target_link_libraries(neo_node_full PRIVATE neo_cpp)
target_include_directories(neo_node_full PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Install targets for node executables
install(TARGETS neo_node_app neo_functional_node neo_node_safe neo_node_production neo_node_production_ready neo_node_full
    RUNTIME DESTINATION bin
)
