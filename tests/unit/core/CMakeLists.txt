# Core unit tests

# Test protocol_settings
add_executable(test_protocol_settings test_protocol_settings.cpp)
target_link_libraries(test_protocol_settings
    PRIVATE
        neo_core
        gtest
        gtest_main
)
target_include_directories(test_protocol_settings
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/third_party/googletest/googletest/include
)
add_test(NAME test_protocol_settings COMMAND test_protocol_settings)

# Additional comprehensive Core tests converted from C#
add_executable(test_core_uint160_complete test_core_uint160_complete.cpp)
target_link_libraries(test_core_uint160_complete
    PRIVATE
        neo_io
        gtest
        gtest_main
)
target_include_directories(test_core_uint160_complete
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/third_party/googletest/googletest/include
)
add_test(NAME test_core_uint160_complete COMMAND test_core_uint160_complete)

add_executable(test_core_uint256_complete test_core_uint256_complete.cpp)
target_link_libraries(test_core_uint256_complete
    PRIVATE
        neo_io
        gtest
        gtest_main
)
target_include_directories(test_core_uint256_complete
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/third_party/googletest/googletest/include
)
add_test(NAME test_core_uint256_complete COMMAND test_core_uint256_complete)

# Comprehensive converted tests from C# reference
add_executable(test_core_comprehensive_converted test_core_comprehensive_converted.cpp)
target_link_libraries(test_core_comprehensive_converted
    PRIVATE
        neo_core
        neo_io
        gtest
        gtest_main
)
target_include_directories(test_core_comprehensive_converted
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/third_party/googletest/googletest/include
)
add_test(NAME test_core_comprehensive_converted COMMAND test_core_comprehensive_converted)