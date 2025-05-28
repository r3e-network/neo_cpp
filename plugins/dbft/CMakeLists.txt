cmake_minimum_required(VERSION 3.15)
project(neo_dbft VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add source files
set(SOURCES
    src/dbft_plugin.cpp
)

# Add header files
set(HEADERS
    include/neo/plugins/dbft/dbft_plugin.h
)

# Create library
add_library(neo_dbft SHARED ${SOURCES} ${HEADERS})

# Set include directories
target_include_directories(neo_dbft
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link dependencies
target_link_libraries(neo_dbft
    PUBLIC
        neo_core
)

# Install
install(TARGETS neo_dbft
    EXPORT neo_dbft-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY include/ DESTINATION include)

install(EXPORT neo_dbft-targets
    FILE neo_dbft-targets.cmake
    NAMESPACE neo::
    DESTINATION lib/cmake/neo_dbft
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/neo_dbft-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_file(cmake/neo_dbft-config.cmake
    "${CMAKE_CURRENT_BINARY_DIR}/neo_dbft-config.cmake"
    COPYONLY
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/neo_dbft-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/neo_dbft-config-version.cmake"
    DESTINATION lib/cmake/neo_dbft
)
