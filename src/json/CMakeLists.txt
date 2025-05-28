# Create the json library
add_library(neo_json OBJECT
    jtoken.cpp
    jobject.cpp
    jarray.cpp
    jstring.cpp
    jnumber.cpp
    jboolean.cpp
    ordered_dictionary.cpp
)

# Include directories
target_include_directories(neo_json
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(neo_json
    PUBLIC
        nlohmann_json::nlohmann_json
)
