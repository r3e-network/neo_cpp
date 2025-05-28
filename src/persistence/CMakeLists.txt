# Create the persistence library
add_library(neo_persistence OBJECT
    memory_store.cpp
    rocksdb_store.cpp
    storage_key.cpp
    storage_item.cpp
    data_cache.cpp
)

# Include directories
target_include_directories(neo_persistence
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies conditionally
if(RocksDB_FOUND)
    target_link_libraries(neo_persistence
        PUBLIC
            RocksDB::rocksdb
    )
    target_compile_definitions(neo_persistence PUBLIC NEO_HAS_ROCKSDB)
else()
    message(WARNING "RocksDB not found - persistence module will use memory store only")
endif()
