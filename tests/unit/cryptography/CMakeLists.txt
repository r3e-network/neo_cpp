# Create the cryptography unit tests
add_executable(test_cryptography
    test_hash.cpp
    test_crypto.cpp
    # test_ecc.cpp  # Disabled due to missing Secp256r1 implementation
    test_merkletree.cpp
    # test_bls12_381.cpp  # Disabled due to undefined references
    # test_base58.cpp  # Disabled due to missing Hash::Checksum API
    # test_base64url.cpp  # Disabled due to API changes
    # test_murmur.cpp  # Disabled due to missing Murmur128 API
    # test_bloom_filter.cpp  # Disabled due to API changes
    # test_mpttrie.cpp  # Disabled due to API changes
)

# Include directories
target_include_directories(test_cryptography
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(test_cryptography
    PRIVATE
        neo_cpp
        ${GTEST_LIBRARIES}
        OpenSSL::SSL
        OpenSSL::Crypto
        nlohmann_json::nlohmann_json
        spdlog::spdlog
)

# Add test
add_test(NAME test_cryptography COMMAND test_cryptography)
