# Security Tests
add_executable(test_security
    test_authentication.cpp
    test_input_validation.cpp
    test_rate_limiting.cpp
    test_session_management.cpp
)

target_link_libraries(test_security
    neo_core
    neo_rpc
    GTest::gtest_main
)

target_include_directories(test_security PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

add_test(NAME SecurityTests COMMAND test_security)
set_tests_properties(SecurityTests PROPERTIES LABELS "security;critical")
