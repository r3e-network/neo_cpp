# Create the Ledger unit tests
add_executable(test_ledger
    test_transaction.cpp
    test_block.cpp
    test_json_serialization.cpp
    test_header_cache.cpp
    test_memory_pool_all_methods_complete.cpp
)

# Include directories
target_include_directories(test_ledger
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(test_ledger
    PRIVATE
        neo_cpp
        ${GTEST_LIBRARIES}
)

# Add test
add_test(NAME test_ledger COMMAND test_ledger)
