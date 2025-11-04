# Create the NEP6 wallet unit tests
add_executable(test_nep6_wallet
    test_nep6_wallet.cpp
    test_scryptparameters.cpp
    # test_nep6_wallet_all_methods_complete.cpp  # Disabled pending API alignment
)

# Include directories
target_include_directories(test_nep6_wallet
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(test_nep6_wallet
    PRIVATE
        neo_cpp
        ${GTEST_LIBRARIES}
)

# Add test
add_test(NAME test_nep6_wallet COMMAND test_nep6_wallet)
