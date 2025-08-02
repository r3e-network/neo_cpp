# Neo Logging Module
cmake_minimum_required(VERSION 3.20)

# Create the logging library
add_library(neo_logging STATIC
    logger.cpp
)

# Set target properties
set_target_properties(neo_logging PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Include directories
target_include_directories(neo_logging
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Find spdlog
find_package(spdlog REQUIRED)
if(spdlog_FOUND)
    # Link dependencies - spdlog is available
    target_link_libraries(neo_logging
        PUBLIC
            spdlog::spdlog
    )
    # Define spdlog availability
    target_compile_definitions(neo_logging PUBLIC NEO_HAS_SPDLOG)
    message(STATUS "Logging: Using spdlog for advanced logging")
else()
    message(FATAL_ERROR "spdlog is required but not found")
endif()

# Compiler-specific options
if(MSVC)
    target_compile_options(neo_logging PRIVATE /W4)
else()
    target_compile_options(neo_logging PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Export the target
set_property(TARGET neo_logging PROPERTY EXPORT_NAME logging)
