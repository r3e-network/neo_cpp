# Create the ledger library
add_library(neo_ledger OBJECT
    witness.cpp
    # coin_reference.cpp - deprecated in Neo N3
    # transaction_output.cpp - deprecated in Neo N3 (UTXO model)
    transaction_attribute.cpp
    # transaction_basic.cpp - deprecated Neo 2.x transaction implementation
    # transaction_serialization.cpp - deprecated Neo 2.x transaction serialization
    # transaction_verification.cpp - deprecated Neo 2.x transaction verification
    # transaction_json.cpp - deprecated Neo 2.x transaction JSON serialization
    block.cpp
    block_json.cpp
    blockchain.cpp
    mempool.cpp
    header_cache.cpp
    pool_item.cpp
    transaction_verification_context.cpp
)

# Include directories
target_include_directories(neo_ledger
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(neo_ledger
    PUBLIC
        neo_io
        neo_cryptography
        neo_persistence
)
