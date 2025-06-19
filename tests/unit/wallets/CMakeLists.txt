# Add subdirectories
add_subdirectory(nep6)

# Create the wallets unit tests
add_executable(test_wallets
    test_wallet.cpp
    test_asset_descriptor.cpp
    test_wallet_transaction.cpp
    test_verification_contract.cpp
    test_helper.cpp
    test_key_pair.cpp
    test_wallet_all_methods_complete.cpp
)

# Include directories
target_include_directories(test_wallets
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(test_wallets
    PRIVATE
        neo_cpp
        ${GTEST_LIBRARIES}
)

# Add test
add_test(NAME test_wallets COMMAND test_wallets)
