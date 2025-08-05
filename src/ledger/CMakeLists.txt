# Neo Ledger Module

# Create ledger library with minimal working files
add_library(neo_ledger OBJECT
    stubs.cpp  # Temporary stubs for linking
    ledger_dummy.cpp
    blockchain_stubs.cpp  # Minimal blockchain stubs
    block.cpp
    block_header.cpp
    header_cache.cpp
    memory_pool_enhanced.cpp
    event_system.cpp
    pool_item.cpp
    transaction_attribute.cpp
    transaction_output.cpp
    coin_reference.cpp
    witness.cpp
    signer.cpp
)

# Set include directories
target_include_directories(neo_ledger PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Link dependencies
target_link_libraries(neo_ledger PUBLIC
    neo_core
    neo_cryptography
    neo_io
)