# Create the blockchain library
add_library(neo_blockchain OBJECT
    header.cpp
)

# Include directories
target_include_directories(neo_blockchain
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(neo_blockchain
    PUBLIC
        neo_io
        neo_ledger
)