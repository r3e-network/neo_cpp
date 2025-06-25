# Create the extensions library
add_library(neo_extensions OBJECT
    base64.cpp
    byte_extensions.cpp
    string_extensions.cpp
    integer_extensions.cpp
    biginteger_extensions.cpp
    datetime_extensions.cpp
    random_extensions.cpp
    collection_extensions.cpp
    hashset_extensions.cpp
    ipaddress_extensions.cpp
    byte_array_comparer.cpp
    byte_array_equality_comparer.cpp
    assembly_extensions.cpp
    secure_string_extensions.cpp
    utility.cpp
)

# Include directories
target_include_directories(neo_extensions
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(neo_extensions
    PUBLIC
        OpenSSL::SSL
        OpenSSL::Crypto
)
