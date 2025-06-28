# Neo Ledger Module

# Create ledger library
add_library(neo_ledger OBJECT
    ledger_dummy.cpp
    block.cpp
    block_header.cpp
    header_cache.cpp
    memory_pool.cpp
    neo2_transaction.cpp
    transaction_attribute.cpp
    transaction_output.cpp
    coin_reference.cpp
    witness.cpp
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