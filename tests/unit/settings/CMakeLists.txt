add_executable(test_settings
    test_settings.cpp
)

target_link_libraries(test_settings
    PRIVATE
        neo_cpp
        gtest
        gtest_main
)

target_include_directories(test_settings
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
)

add_test(NAME test_settings COMMAND test_settings)
