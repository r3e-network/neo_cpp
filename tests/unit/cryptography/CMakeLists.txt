# Cryptography Test Module CMakeLists.txt

set(CRYPTO_TEST_SOURCES
    test_crypto.cpp
    test_crypto_extended.cpp
    test_crypto_extended_fixed.cpp
    test_hash.cpp
    test_ecdsa_comprehensive.cpp
    test_cryptography_complete.cpp
    # test_key_pair.cpp  # Missing file
    test_merkle_tree.cpp
    test_base58.cpp
    # test_aes.cpp  # Missing file
    test_scrypt.cpp
)

# Main cryptography test executable
add_executable(test_cryptography ${CRYPTO_TEST_SOURCES})

target_link_libraries(test_cryptography
    PRIVATE
        neo_cryptography
        neo_io
        OpenSSL::SSL
        OpenSSL::Crypto
        gtest
        gtest_main
)

target_include_directories(test_cryptography
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/tests/include
        ${CMAKE_SOURCE_DIR}/third_party/googletest/googletest/include
)

# Individual comprehensive test executables
add_executable(test_ecdsa_comprehensive test_ecdsa_comprehensive.cpp)
target_link_libraries(test_ecdsa_comprehensive
    PRIVATE
        neo_cryptography
        neo_io
        OpenSSL::SSL
        OpenSSL::Crypto
        gtest
        gtest_main
)

add_executable(test_cryptography_complete test_cryptography_complete.cpp)
target_link_libraries(test_cryptography_complete
    PRIVATE
        neo_cryptography
        neo_io
        OpenSSL::SSL
        OpenSSL::Crypto
        gtest
        gtest_main
)

# Add tests to CTest
add_test(NAME crypto_basic COMMAND test_cryptography)
add_test(NAME crypto_ecdsa_comprehensive COMMAND test_ecdsa_comprehensive)
add_test(NAME crypto_complete COMMAND test_cryptography_complete)

# Set test properties
set_tests_properties(crypto_basic PROPERTIES TIMEOUT 30)
set_tests_properties(crypto_ecdsa_comprehensive PROPERTIES TIMEOUT 60)
set_tests_properties(crypto_complete PROPERTIES TIMEOUT 60)