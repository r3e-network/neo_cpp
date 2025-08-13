# Create the monitoring library
add_library(neo_monitoring OBJECT
    performance_monitor.cpp
)

# Include directories
target_include_directories(neo_monitoring
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(neo_monitoring
    PUBLIC
        neo_logging
)

# Platform-specific definitions
if(APPLE)
    target_compile_definitions(neo_monitoring PRIVATE __APPLE__)
elseif(UNIX)
    target_compile_definitions(neo_monitoring PRIVATE __linux__)
endif()