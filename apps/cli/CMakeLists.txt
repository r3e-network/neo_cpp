# Create the CLI executable
add_executable(neo-cli
    main.cpp
)

# Include directories
target_include_directories(neo-cli
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(neo-cli
    PRIVATE
        neo_cpp
)

# Install targets
install(TARGETS neo-cli
    RUNTIME DESTINATION bin
)
