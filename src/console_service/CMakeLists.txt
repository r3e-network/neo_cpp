# Create the console_service library
add_library(neo_console_service OBJECT
    console_service_base.cpp
    console_helper.cpp
    command_token.cpp
    command_string_token.cpp
    command_space_token.cpp
    command_quote_token.cpp
    console_color_set.cpp
    console_command_attribute.cpp
    console_command_method.cpp
    service_proxy.cpp
)

# Include directories
target_include_directories(neo_console_service
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(neo_console_service
    PUBLIC
        neo_extensions
)

# Platform-specific libraries
if(WIN32)
    target_link_libraries(neo_console_service
        PRIVATE
            advapi32
    )
endif()
