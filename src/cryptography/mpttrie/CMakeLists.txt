# Create the mpttrie library
add_library(neo_mpttrie OBJECT
    node.cpp
    trie.cpp
    cache.cpp
    trie_get.cpp
    trie_put.cpp
    trie_delete.cpp
    trie_proof.cpp
)

# Include directories
target_include_directories(neo_mpttrie
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(neo_mpttrie
    PUBLIC
        neo_io
        neo_persistence
)
