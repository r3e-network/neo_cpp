# Create the statistics plugin library
add_library(neo_statistics_plugin OBJECT
    statistics_plugin.cpp
    statistics_plugin_factory.cpp
)

# Include directories
target_include_directories(neo_statistics_plugin
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(neo_statistics_plugin
    PUBLIC
        neo_node
        neo_rpc
        neo_plugins
        nlohmann_json::nlohmann_json
)
