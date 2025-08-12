cmake_minimum_required(VERSION 3.16)

# Wallet example
add_executable(wallet_example wallet_example.cpp)
target_link_libraries(wallet_example PRIVATE neo-sdk)

# Transaction example
add_executable(transaction_example transaction_example.cpp)
target_link_libraries(transaction_example PRIVATE neo-sdk)

# Simple example
add_executable(simple_example simple_example.cpp)
target_link_libraries(simple_example PRIVATE neo-sdk)

# Install examples
install(TARGETS 
    wallet_example 
    transaction_example
    simple_example
    DESTINATION ${CMAKE_INSTALL_BINDIR}/examples
)