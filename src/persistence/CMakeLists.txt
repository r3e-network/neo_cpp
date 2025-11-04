# Create the persistence library
set(PERSISTENCE_SOURCES
    memory_store.cpp
    memory_store_view.cpp
    file_store.cpp
    storage_key.cpp
    storage_item.cpp
    data_cache.cpp
    store_cache_complete.cpp
    store_factory_complete.cpp
    snapshot.cpp
)

# Add RocksDB store if available
find_package(RocksDB QUIET)
if(RocksDB_FOUND)
    list(APPEND PERSISTENCE_SOURCES rocksdb_store.cpp)
    message(STATUS "RocksDB found - enabling RocksDB storage backend")
endif()

add_library(neo_persistence OBJECT ${PERSISTENCE_SOURCES})

# Include directories
target_include_directories(neo_persistence
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
if(RocksDB_FOUND)
    target_link_libraries(neo_persistence
        PUBLIC
            RocksDB::rocksdb
    )
    target_compile_definitions(neo_persistence PUBLIC NEO_HAS_ROCKSDB)
    message(STATUS "RocksDB integration enabled")
else()
    message(STATUS "Using memory store only - RocksDB not found")
endif()
