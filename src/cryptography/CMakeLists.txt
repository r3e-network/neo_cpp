# Create the cryptography library
add_library(neo_cryptography OBJECT
    hash.cpp
    crypto.cpp
    ecc.cpp
    ecc/ecpoint.cpp
    merkletree.cpp
    base64.cpp
    base64url.cpp
)

# Optional BLS12_381 support
option(ENABLE_BLS12_381 "Enable BLS12_381 cryptography support" OFF)
if(ENABLE_BLS12_381)
    target_sources(neo_cryptography PRIVATE bls12_381.cpp)
    target_compile_definitions(neo_cryptography PUBLIC NEO_ENABLE_BLS12_381)

    # Try to find blst library
    find_library(BLST_LIBRARY blst)
    if(BLST_LIBRARY)
        target_link_libraries(neo_cryptography PUBLIC ${BLST_LIBRARY})
        target_compile_definitions(neo_cryptography PRIVATE HAVE_BLST)
    else()
        message(WARNING "blst library not found, using mock BLS12_381 implementation")
    endif()
endif()

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
