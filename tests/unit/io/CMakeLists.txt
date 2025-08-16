# IO Test Module CMakeLists.txt

set(IO_TEST_SOURCES
    test_byte_vector.cpp
    test_uint160.cpp
    test_uint_simple.cpp
    test_fixed8.cpp
    test_serialization.cpp
    test_json_serialization.cpp
    test_lru_cache.cpp
    test_io_helper.cpp
    test_caching.cpp
    test_io_simple.cpp
    test_io_extended.cpp
    test_binary_reader.cpp
    test_binary_writer.cpp
)

# Main IO test executable
add_executable(test_io ${IO_TEST_SOURCES})

target_link_libraries(test_io
    PRIVATE
        neo_io
        neo_cryptography
        OpenSSL::SSL
        OpenSSL::Crypto
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        GTest::GTest
        GTest::Main
)

target_include_directories(test_io
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/tests/include
)

# Individual comprehensive test executables
add_executable(test_io_extended test_io_extended.cpp)
target_link_libraries(test_io_extended
    PRIVATE
        neo_io
        GTest::GTest
        GTest::Main
)

# Add tests to CTest
add_test(NAME io_basic COMMAND test_io)
add_test(NAME io_extended COMMAND test_io_extended)

# Set test properties
set_tests_properties(io_basic PROPERTIES TIMEOUT 30)
set_tests_properties(io_extended PROPERTIES TIMEOUT 60)
