# Cryptography Test Module CMakeLists.txt

option(NEO_ENABLE_FULL_CRYPTO_TESTS "Build the legacy aggregate cryptography test harness" OFF)
option(NEO_ENABLE_CRYPTO_SCRYPT_TESTS "Build scrypt unit tests (requires Scrypt implementation)" OFF)

set(CRYPTO_TEST_STUBS
    ${CMAKE_SOURCE_DIR}/tests/unit/io/logger_stub.cpp
    ${CMAKE_SOURCE_DIR}/tests/unit/io/fixed8_stub.cpp
)

if(NEO_ENABLE_FULL_CRYPTO_TESTS)
    set(CRYPTO_TEST_SOURCES
        test_crypto.cpp
        test_hash.cpp
        # test_key_pair.cpp  # Missing file
        test_merkle_tree.cpp
        test_base58.cpp
        # test_aes.cpp  # Missing file
        test_scrypt.cpp
    )

    add_executable(test_cryptography ${CRYPTO_TEST_SOURCES})

    target_link_libraries(test_cryptography
        PRIVATE
            neo_cpp
            gtest
            gtest_main
            gmock
    )

    target_include_directories(test_cryptography
        PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/tests/include
            ${CMAKE_SOURCE_DIR}/third_party/googletest/googletest/include
            ${CMAKE_SOURCE_DIR}/third_party/googletest/googlemock/include
    )

    add_test(NAME crypto_basic COMMAND test_cryptography)
    set_tests_properties(crypto_basic PROPERTIES TIMEOUT 30)
endif()

# Targeted signing regression tests
add_executable(test_crypto_sign test_crypto_sign.cpp)
target_link_libraries(test_crypto_sign
    PRIVATE
        neo_cpp
        gtest
        gtest_main
)

target_include_directories(test_crypto_sign
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/tests/include
        ${CMAKE_SOURCE_DIR}/third_party/googletest/googletest/include
)

target_sources(test_crypto_sign
    PRIVATE
        ${CRYPTO_TEST_STUBS}
)

add_test(NAME crypto_sign COMMAND test_crypto_sign)
set_tests_properties(crypto_sign PROPERTIES TIMEOUT 15)

# Base58 focus tests
add_executable(test_crypto_base58 test_base58.cpp)
target_link_libraries(test_crypto_base58
    PRIVATE
        neo_cpp
        gtest
        gtest_main
)

target_include_directories(test_crypto_base58
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/tests/include
        ${CMAKE_SOURCE_DIR}/third_party/googletest/googletest/include
)

target_sources(test_crypto_base58
    PRIVATE
        ${CRYPTO_TEST_STUBS}
)

add_test(NAME crypto_base58 COMMAND test_crypto_base58)
set_tests_properties(crypto_base58 PROPERTIES TIMEOUT 15)

if(NEO_ENABLE_CRYPTO_SCRYPT_TESTS)
    add_executable(test_crypto_scrypt test_scrypt.cpp)
    target_link_libraries(test_crypto_scrypt
        PRIVATE
            neo_cpp
            gtest
            gtest_main
    )

    target_include_directories(test_crypto_scrypt
        PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/tests/include
            ${CMAKE_SOURCE_DIR}/third_party/googletest/googletest/include
    )

    target_sources(test_crypto_scrypt
        PRIVATE
            ${CRYPTO_TEST_STUBS}
    )

    add_test(NAME crypto_scrypt COMMAND test_crypto_scrypt)
    set_tests_properties(crypto_scrypt PROPERTIES TIMEOUT 30)
endif()

# Merkle tree focus tests
add_executable(test_crypto_merkle test_merkle_tree.cpp)
target_link_libraries(test_crypto_merkle
    PRIVATE
        neo_cpp
        gtest
        gtest_main
)

target_include_directories(test_crypto_merkle
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/tests/include
        ${CMAKE_SOURCE_DIR}/third_party/googletest/googletest/include
)

target_sources(test_crypto_merkle
    PRIVATE
        ${CRYPTO_TEST_STUBS}
)

add_test(NAME crypto_merkle COMMAND test_crypto_merkle)
set_tests_properties(crypto_merkle PROPERTIES TIMEOUT 15)

# Extended cryptography coverage (converted from C#)
add_executable(test_crypto_extended test_crypto_extended.cpp)
target_link_libraries(test_crypto_extended
    PRIVATE
        neo_cpp
        gtest
        gtest_main
)

target_include_directories(test_crypto_extended
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/tests/include
        ${CMAKE_SOURCE_DIR}/third_party/googletest/googletest/include
)

add_test(NAME crypto_extended COMMAND test_crypto_extended)
set_tests_properties(crypto_extended PROPERTIES TIMEOUT 30)
