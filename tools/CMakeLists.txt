cmake_minimum_required(VERSION 3.15)
project(neo_tools VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add tool executables
add_executable(neo_cli_tool cli/main.cpp)
target_link_libraries(neo_cli_tool PRIVATE neo_core)

add_executable(neo_gui gui/main.cpp)
target_link_libraries(neo_gui PRIVATE neo_core)

# Install
install(TARGETS
    neo_cli_tool
    neo_gui
    RUNTIME DESTINATION bin
)
