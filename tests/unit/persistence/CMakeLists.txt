# Create the Persistence unit tests
add_executable(test_persistence
    test_memory_store.cpp
    test_storage_key.cpp
    test_storage_item.cpp
    test_data_cache.cpp
    test_cloned_cache.cpp
    test_store_cache.cpp
)

# Include directories
target_include_directories(test_persistence
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(test_persistence
    PRIVATE
        neo_cpp
        ${GTEST_LIBRARIES}
)

# Add test
add_test(NAME test_persistence COMMAND test_persistence)
