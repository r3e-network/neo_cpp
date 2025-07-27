# Create the persistence library
set(PERSISTENCE_SOURCES
    memory_store.cpp
    memory_store_view.cpp
    storage_key.cpp
    storage_item.cpp
    data_cache.cpp
    store_factory.cpp
    # store_cache_complete.cpp  # Disabled - duplicate symbols with data_cache.cpp
    # store_factory_complete.cpp  # Disabled - duplicate symbols with store_factory.cpp
)

# Add RocksDB store only if RocksDB is found
if(RocksDB_FOUND)
    list(APPEND PERSISTENCE_SOURCES rocksdb_store.cpp)
endif()

add_library(neo_persistence OBJECT ${PERSISTENCE_SOURCES})

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
