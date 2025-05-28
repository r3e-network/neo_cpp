# Create the IO unit tests
add_executable(test_io
    test_byte_vector.cpp
    test_uint160.cpp
    test_fixed8.cpp
    test_serialization.cpp
    test_json_serialization.cpp
    test_lru_cache.cpp
    test_io_helper.cpp
    test_caching.cpp
)

# Include directories
target_include_directories(test_io
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(test_io
    PRIVATE
        neo_cpp
        ${GTEST_LIBRARIES}
        OpenSSL::SSL
        OpenSSL::Crypto
        nlohmann_json::nlohmann_json
        spdlog::spdlog
)

# Add test
add_test(NAME test_io COMMAND test_io)
