# Create the CLI unit tests
add_executable(test_cli
    test_cli.cpp
    test_command_handler.cpp
    test_type_converters.cpp
)

# Link libraries
target_link_libraries(test_cli
    PRIVATE
        neo_cpp
        ${GTEST_LIBRARIES}
)

# Add include directories
target_include_directories(test_cli
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

# Add test
add_test(NAME test_cli COMMAND test_cli)
