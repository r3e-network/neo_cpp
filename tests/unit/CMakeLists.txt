# Add subdirectories
add_subdirectory(cryptography)
add_subdirectory(io)
add_subdirectory(json)
add_subdirectory(extensions)
add_subdirectory(console_service)
add_subdirectory(persistence)
add_subdirectory(network)
add_subdirectory(ledger)
add_subdirectory(vm)
add_subdirectory(smartcontract)
add_subdirectory(wallets)
add_subdirectory(plugins)
add_subdirectory(consensus)
add_subdirectory(rpc)
add_subdirectory(cli)
add_subdirectory(monitoring)

# Main unit tests
# add_executable(test_main
#     test_protocol_settings_all_methods_complete.cpp
# )

# target_include_directories(test_main
#     PRIVATE
#         ${CMAKE_SOURCE_DIR}/include
#         ${CMAKE_CURRENT_SOURCE_DIR}
# )

# target_link_libraries(test_main
#     PRIVATE
#         neo_cpp
#         ${GTEST_LIBRARIES}
# )

# add_test(NAME test_main COMMAND test_main)
