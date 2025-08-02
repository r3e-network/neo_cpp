cmake_minimum_required(VERSION 3.15)
project(neo_tools VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add tool executables
add_executable(neo_cli_tool cli/main.cpp)
target_link_libraries(neo_cli_tool PRIVATE 
    neo_cpp
    neo_plugins
)

add_executable(neo_gui gui/main.cpp)
target_link_libraries(neo_gui PRIVATE 
    neo_cpp
    neo_plugins
)

# RPC server test tool
add_executable(test_rpc_server test_rpc_server.cpp)
target_link_libraries(test_rpc_server PRIVATE 
    neo_cpp
    neo_plugins
)

# Install
install(TARGETS
    neo_cli_tool
    neo_gui
    test_rpc_server
    RUNTIME DESTINATION bin
)
