cmake_minimum_required(VERSION 3.15)
project(neo_examples VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add example executables
add_executable(neo_example_vm vm/vm_example.cpp)
target_link_libraries(neo_example_vm PRIVATE 
    neo_vm 
    $<TARGET_OBJECTS:neo_smartcontract>
    $<TARGET_OBJECTS:neo_cryptography>
    $<TARGET_OBJECTS:neo_persistence>
    $<TARGET_OBJECTS:neo_network>
    $<TARGET_OBJECTS:neo_extensions>
    neo_core 
    neo_io 
    neo_native_contracts 
    neo_logging
    neo_ledger
    neo_plugins
    neo_wallets
    OpenSSL::SSL
    OpenSSL::Crypto)

add_executable(neo_example_smartcontract smartcontract/smartcontract_example.cpp)
target_link_libraries(neo_example_smartcontract PRIVATE 
    $<TARGET_OBJECTS:neo_smartcontract>
    $<TARGET_OBJECTS:neo_cryptography>
    $<TARGET_OBJECTS:neo_persistence>
    $<TARGET_OBJECTS:neo_network>
    $<TARGET_OBJECTS:neo_extensions>
    neo_io 
    neo_vm 
    neo_ledger 
    neo_logging 
    neo_core 
    neo_native_contracts
    neo_plugins
    neo_wallets
    OpenSSL::SSL
    OpenSSL::Crypto)

add_executable(neo_example_network network/network_example.cpp)
target_link_libraries(neo_example_network PRIVATE 
    $<TARGET_OBJECTS:neo_network>
    $<TARGET_OBJECTS:neo_extensions>
    neo_ledger
    neo_core
    neo_io
    $<TARGET_OBJECTS:neo_cryptography>
    $<TARGET_OBJECTS:neo_persistence>
    $<TARGET_OBJECTS:neo_smartcontract>
    neo_vm
    neo_native_contracts
    neo_logging
    neo_plugins
    neo_wallets
    OpenSSL::SSL
    OpenSSL::Crypto)

add_executable(neo_example_consensus consensus/consensus_example.cpp)
target_link_libraries(neo_example_consensus PRIVATE 
    neo_consensus
    $<TARGET_OBJECTS:neo_network>
    $<TARGET_OBJECTS:neo_extensions>
    neo_ledger
    neo_core
    neo_io
    $<TARGET_OBJECTS:neo_cryptography>
    $<TARGET_OBJECTS:neo_persistence>
    $<TARGET_OBJECTS:neo_smartcontract>
    neo_vm
    neo_native_contracts
    neo_logging
    neo_plugins
    neo_wallets
    OpenSSL::SSL
    OpenSSL::Crypto)

add_executable(neo_example_ledger ledger/ledger_example.cpp)
target_link_libraries(neo_example_ledger PRIVATE 
    neo_ledger 
    neo_core 
    neo_io 
    $<TARGET_OBJECTS:neo_cryptography>
    $<TARGET_OBJECTS:neo_extensions>
    neo_vm 
    $<TARGET_OBJECTS:neo_smartcontract>
    $<TARGET_OBJECTS:neo_persistence>
    $<TARGET_OBJECTS:neo_network>
    neo_native_contracts 
    neo_logging
    neo_plugins
    neo_wallets
    OpenSSL::SSL
    OpenSSL::Crypto)

add_executable(neo_example_event_system event_system_usage.cpp)
target_link_libraries(neo_example_event_system PRIVATE 
    neo_ledger 
    neo_core 
    neo_io 
    neo_logging 
    $<TARGET_OBJECTS:neo_extensions>
    $<TARGET_OBJECTS:neo_persistence>
    $<TARGET_OBJECTS:neo_smartcontract>
    $<TARGET_OBJECTS:neo_cryptography>
    $<TARGET_OBJECTS:neo_network>
    neo_native_contracts
    neo_vm
    neo_plugins
    neo_wallets
    OpenSSL::SSL
    OpenSSL::Crypto)
target_include_directories(neo_example_event_system PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Install
install(TARGETS
    neo_example_vm
    neo_example_smartcontract
    neo_example_network
    neo_example_consensus
    neo_example_ledger
    neo_example_event_system
    RUNTIME DESTINATION bin/examples
)
