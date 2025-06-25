# Create the CLI executable
add_executable(neo_cli_app
    main.cpp
)

# Set target properties
set_target_properties(neo_cli_app PROPERTIES
    OUTPUT_NAME "neo-cli"
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

# Include directories
target_include_directories(neo_cli_app
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link with all required libraries
target_link_libraries(neo_cli_app
    PRIVATE
        neo_cpp
        ${BOOST_LIBRARIES}
        OpenSSL::SSL
        OpenSSL::Crypto
        nlohmann_json::nlohmann_json
        spdlog::spdlog
)

# Install targets
install(TARGETS neo_cli_app
    RUNTIME DESTINATION bin
)
