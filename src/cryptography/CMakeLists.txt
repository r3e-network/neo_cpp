# Create the cryptography library
add_library(neo_cryptography OBJECT
    hash.cpp
    crypto.cpp
    ecc.cpp
    ecc/ecpoint.cpp
    ecc/ecdsa.cpp
    ecrecover.cpp
    merkletree.cpp
    base64.cpp
    base64url.cpp
    base58.cpp
    lz4.cpp
)

# BLS12_381 support (always enabled for Neo compatibility)
target_sources(neo_cryptography PRIVATE bls12_381_complete.cpp)
target_compile_definitions(neo_cryptography PUBLIC NEO_ENABLE_BLS12_381)

# Add MPTTrie subdirectory
add_subdirectory(mpttrie)

# Include directories
target_include_directories(neo_cryptography
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(neo_cryptography
    PUBLIC
        OpenSSL::SSL
        OpenSSL::Crypto
)
