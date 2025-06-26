# Neo Ledger Module

# Create ledger library
add_library(neo_ledger OBJECT
    ledger_dummy.cpp
    # block.cpp # Temporarily disabled due to transaction dependencies
    # memory_pool.cpp # Temporarily disabled due to transaction dependencies
    # neo2_transaction.cpp # Temporarily disabled due to API mismatches
    # transaction_attribute.cpp # Temporarily disabled
    # transaction_output.cpp # Temporarily disabled
    # coin_reference.cpp # Temporarily disabled
    # witness.cpp # Temporarily disabled
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