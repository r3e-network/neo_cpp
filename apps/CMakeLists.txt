# Add the node subdirectory
add_subdirectory(node)

# Add the CLI subdirectory
add_subdirectory(cli)

# Add the neo-cli subdirectory
add_subdirectory(neo-cli)

# Create production neo node application
add_executable(neo_node_app neo_node.cpp)
target_link_libraries(neo_node_app PRIVATE neo_cpp)
target_include_directories(neo_node_app PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Create production neo node application
add_executable(neo_node_production neo_node_production.cpp)
target_link_libraries(neo_node_production PRIVATE neo_cpp)
target_include_directories(neo_node_production PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Create working neo node application  
add_executable(working_neo_node working_neo_node.cpp)
target_link_libraries(working_neo_node PRIVATE neo_cpp)
target_include_directories(working_neo_node PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Create BLS12-381 test application
add_executable(test_bls12_381 test_bls12_381.cpp)
target_link_libraries(test_bls12_381 PRIVATE neo_cpp)
target_include_directories(test_bls12_381 PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Create testnet node
add_executable(testnet_node testnet_node.cpp)
target_link_libraries(testnet_node PRIVATE neo_logging neo_core neo_rpc neo_persistence neo_cryptography neo_io)
target_include_directories(testnet_node PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Create complete testnet node
add_executable(neo_testnet_complete neo_testnet_complete.cpp)
target_link_libraries(neo_testnet_complete PRIVATE neo_logging neo_cryptography neo_io neo_extensions neo_core)
target_include_directories(neo_testnet_complete PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Create standalone testnet node (minimal dependencies)
add_executable(neo_testnet_standalone neo_testnet_standalone.cpp)
target_link_libraries(neo_testnet_standalone PRIVATE neo_cryptography neo_io neo_extensions neo_core)
target_include_directories(neo_testnet_standalone PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Create crypto demo (minimal dependencies)
add_executable(neo_crypto_demo neo_crypto_demo.cpp)
target_link_libraries(neo_crypto_demo PRIVATE neo_cryptography neo_io neo_extensions neo_core)
target_include_directories(neo_crypto_demo PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Create BLS demo (truly minimal dependencies)
add_executable(bls_demo bls_demo.cpp)
target_link_libraries(bls_demo PRIVATE neo_cryptography neo_io neo_extensions neo_core)
target_include_directories(bls_demo PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Create minimal neo node (reduced dependencies)
add_executable(minimal_neo_node minimal_neo_node.cpp)
target_link_libraries(minimal_neo_node PRIVATE neo_cryptography neo_io neo_extensions neo_core neo_persistence neo_smartcontract neo_vm neo_native_contracts)
target_include_directories(minimal_neo_node PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Create simple test node application
add_executable(test_simple_node test_simple_node.cpp)
target_link_libraries(test_simple_node PRIVATE neo_cpp)
target_include_directories(test_simple_node PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Create testnet node application
add_executable(neo_testnet_node neo_testnet_node.cpp)
target_link_libraries(neo_testnet_node PRIVATE neo_cpp)
target_include_directories(neo_testnet_node PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Create fixed node application (no threading issues)
add_executable(neo_node_fixed neo_node_fixed.cpp)
target_link_libraries(neo_node_fixed PRIVATE neo_cpp)
target_include_directories(neo_node_fixed PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Install targets for node executables
install(TARGETS neo_node_app working_neo_node neo_testnet_node neo_node_fixed
    RUNTIME DESTINATION bin
)
