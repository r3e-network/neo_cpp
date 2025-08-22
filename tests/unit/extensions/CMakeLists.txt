# Extensions unit tests
add_executable(test_extensions
    test_byte_extensions.cpp
    test_string_extensions.cpp
    test_integer_extensions.cpp
)

target_link_libraries(test_extensions
    PRIVATE
        neo_cpp
        gtest
        gtest_main
)

target_include_directories(test_extensions
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
)

# Add test to CTest
add_test(NAME ExtensionsTests COMMAND test_extensions)

# Additional comprehensive test
add_executable(test_extensions_comprehensive_converted test_extensions_comprehensive_converted.cpp)
target_link_libraries(test_extensions_comprehensive_converted
    PRIVATE
        neo_core
        gtest
        gtest_main
)
add_test(NAME test_extensions_comprehensive_converted COMMAND test_extensions_comprehensive_converted)
