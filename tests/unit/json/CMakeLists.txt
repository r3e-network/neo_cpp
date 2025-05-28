# JSON unit tests
add_executable(test_json
    test_jobject.cpp
    test_jarray.cpp
    test_jstring.cpp
    test_jnumber.cpp
    test_jboolean.cpp
    test_jtoken.cpp
)

target_link_libraries(test_json
    PRIVATE
        neo_cpp
        gtest
        gtest_main
        nlohmann_json::nlohmann_json
)

target_include_directories(test_json
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
)

# Add test to CTest
add_test(NAME JsonTests COMMAND test_json)
