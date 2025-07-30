# Console Service unit tests
add_executable(test_console_service
    test_console_helper.cpp
    test_command_token.cpp
    test_console_service_base.cpp
)

target_link_libraries(test_console_service
    PRIVATE
        neo_console_service
        neo_core
        GTest::gtest
        GTest::gtest_main
)

target_include_directories(test_console_service
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
)

# Add test to CTest
add_test(NAME ConsoleServiceTests COMMAND test_console_service)
