# Create the ledger library
add_library(neo_ledger OBJECT
    witness.cpp
    coin_reference.cpp
    transaction_output.cpp
    transaction_attribute.cpp
    transaction_basic.cpp
    transaction_serialization.cpp
    transaction_verification.cpp
    transaction_json.cpp
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
