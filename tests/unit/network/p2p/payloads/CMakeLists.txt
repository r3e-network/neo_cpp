# Create the payloads unit tests
add_executable(test_payloads
    test_transaction_all_methods_complete.cpp
)

# Include directories
target_include_directories(test_payloads
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(test_payloads
    PRIVATE
        neo_cpp
        ${GTEST_LIBRARIES}
)

# Add test
add_test(NAME test_payloads COMMAND test_payloads)