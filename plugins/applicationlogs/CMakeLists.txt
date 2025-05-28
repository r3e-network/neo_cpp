cmake_minimum_required(VERSION 3.15)
project(neo_applicationlogs VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add source files
set(SOURCES
    src/applicationlogs_plugin.cpp
)

# Add header files
set(HEADERS
    include/neo/plugins/applicationlogs/applicationlogs_plugin.h
)

# Create library
add_library(neo_applicationlogs SHARED ${SOURCES} ${HEADERS})

# Set include directories
target_include_directories(neo_applicationlogs
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link dependencies
target_link_libraries(neo_applicationlogs
    PUBLIC
        neo_core
)

# Install
install(TARGETS neo_applicationlogs
    EXPORT neo_applicationlogs-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY include/ DESTINATION include)

install(EXPORT neo_applicationlogs-targets
    FILE neo_applicationlogs-targets.cmake
    NAMESPACE neo::
    DESTINATION lib/cmake/neo_applicationlogs
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/neo_applicationlogs-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_file(cmake/neo_applicationlogs-config.cmake
    "${CMAKE_CURRENT_BINARY_DIR}/neo_applicationlogs-config.cmake"
    COPYONLY
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/neo_applicationlogs-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/neo_applicationlogs-config-version.cmake"
    DESTINATION lib/cmake/neo_applicationlogs
)
