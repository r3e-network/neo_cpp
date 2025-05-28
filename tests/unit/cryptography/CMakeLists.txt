# Create the cryptography unit tests
add_executable(test_cryptography
    test_hash.cpp
    test_crypto.cpp
    test_ecc.cpp
    test_merkletree.cpp
    test_bls12_381.cpp
    test_base58.cpp
    test_base64url.cpp
    test_murmur.cpp
    test_bloom_filter.cpp
    test_mpttrie.cpp
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
