# Create the io library
add_library(neo_io OBJECT
    binary_reader.cpp
    binary_writer.cpp
    json_reader.cpp
    json_writer.cpp
    ijson_serializable.cpp
    iserializable.cpp
    byte_vector.cpp
    byte_span.cpp
    uint160.cpp
    uint256.cpp
    fixed8.cpp
    lru_cache.cpp
    memory_stream.cpp
)

# Include directories
target_include_directories(neo_io
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(neo_io
    PUBLIC
        nlohmann_json::nlohmann_json
)
