# Create the payloads unit tests
add_executable(test_payloads
    test_transaction_attributes_json.cpp
    test_addrpayload.cpp
    test_networkaddresswithtime.cpp
)

# Include directories
target_include_directories(test_payloads
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/tests/include
)

# Link dependencies
target_link_libraries(test_payloads
    PRIVATE
        neo_cpp
        gtest
        gtest_main
)

# Add test
add_test(NAME test_payloads COMMAND test_payloads)
