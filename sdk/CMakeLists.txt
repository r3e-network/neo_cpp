cmake_minimum_required(VERSION 3.16)
project(neo-cpp-sdk VERSION 1.0.0 LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(NEO_SDK_BUILD_SHARED "Build shared library" ON)
option(NEO_SDK_BUILD_EXAMPLES "Build example programs" ON)
option(NEO_SDK_BUILD_TESTS "Build unit tests" ON)
option(NEO_SDK_BUILD_DOCS "Build documentation" OFF)

# Add parent neo_cpp as dependency
set(NEO_CPP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)
if(NOT TARGET neo_cpp)
    add_subdirectory(${NEO_CPP_DIR} ${CMAKE_CURRENT_BINARY_DIR}/neo_cpp)
endif()

# SDK library sources
set(NEO_SDK_SOURCES
    src/sdk.cpp
    src/core/blockchain.cpp
    src/core/types.cpp
    src/wallet/wallet.cpp
    # src/wallet/account.cpp  # To be implemented
    src/tx/transaction_builder.cpp
    # src/contract/contract_deployer.cpp  # To be implemented
    # src/contract/contract_invoker.cpp  # To be implemented
    # src/contract/nep17_token.cpp  # To be implemented
    src/rpc/rpc_client.cpp
    # src/network/network_client.cpp  # To be implemented
    # src/crypto/keypair.cpp  # To be implemented
    # src/crypto/hash.cpp  # To be implemented
    # src/storage/blockchain_storage.cpp  # To be implemented
    # src/utils/converter.cpp  # To be implemented
)

# SDK headers
set(NEO_SDK_HEADERS
    include/neo/sdk.h
    include/neo/sdk/core/blockchain.h
    include/neo/sdk/core/types.h
    include/neo/sdk/wallet/wallet.h
    include/neo/sdk/wallet/account.h
    include/neo/sdk/tx/transaction_builder.h
    include/neo/sdk/contract/contract_deployer.h
    include/neo/sdk/contract/contract_invoker.h
    include/neo/sdk/contract/nep17_token.h
    include/neo/sdk/rpc/rpc_client.h
    include/neo/sdk/network/network_client.h
    include/neo/sdk/crypto/keypair.h
    include/neo/sdk/crypto/hash.h
    include/neo/sdk/storage/blockchain_storage.h
    include/neo/sdk/utils/converter.h
)

# Create SDK library
if(NEO_SDK_BUILD_SHARED)
    add_library(neo-sdk SHARED ${NEO_SDK_SOURCES})
    set_target_properties(neo-sdk PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
    )
else()
    add_library(neo-sdk STATIC ${NEO_SDK_SOURCES})
endif()

# Configure target
target_include_directories(neo-sdk
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${NEO_CPP_DIR}/include
)

# Find required packages
find_package(CURL REQUIRED)
find_package(nlohmann_json QUIET)

# Link with neo_cpp library and dependencies
target_link_libraries(neo-sdk
    PUBLIC
        neo_cpp
        CURL::libcurl
    PRIVATE
        $<$<TARGET_EXISTS:nlohmann_json::nlohmann_json>:nlohmann_json::nlohmann_json>
)

# Set properties
set_target_properties(neo-sdk PROPERTIES
    PUBLIC_HEADER "${NEO_SDK_HEADERS}"
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

# Installation rules
include(GNUInstallDirs)

install(TARGETS neo-sdk
    EXPORT neo-sdk-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/neo/sdk
)

install(DIRECTORY include/neo
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)

# Export targets
install(EXPORT neo-sdk-targets
    FILE neo-sdk-targets.cmake
    NAMESPACE neo::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/neo-sdk
)

# Create config file
include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/neo-sdk-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/neo-sdk-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/neo-sdk
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/neo-sdk-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/neo-sdk-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/neo-sdk-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/neo-sdk
)

# Build examples
if(NEO_SDK_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Build tests
if(NEO_SDK_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Build documentation
if(NEO_SDK_BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
        set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
        
        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
        
        add_custom_target(neo-sdk-docs ALL
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM
        )
    endif()
endif()

# Print configuration summary
message(STATUS "Neo C++ SDK Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Shared library: ${NEO_SDK_BUILD_SHARED}")
message(STATUS "  Examples: ${NEO_SDK_BUILD_EXAMPLES}")
message(STATUS "  Tests: ${NEO_SDK_BUILD_TESTS}")
message(STATUS "  Documentation: ${NEO_SDK_BUILD_DOCS}")