cmake_minimum_required(VERSION 3.20)

# Project configuration
project(Neo-CPP 
    VERSION 1.0.0
    DESCRIPTION "Production-ready Neo blockchain node implementation in C++"
    LANGUAGES CXX
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type configuration
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# Compiler-specific options
if(NOT MSVC)
    # GCC/Clang options with relaxed settings
    add_compile_options(-Wall -Wno-error -Wno-unused-parameter -Wno-unused-variable -Wno-unknown-pragmas)
    add_compile_options(-I${CMAKE_CURRENT_SOURCE_DIR}/include)
    
    # Debug options
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g -O0)
    endif()
endif()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Find system packages (optional)
find_package(Threads REQUIRED)
find_package(OpenSSL QUIET)

# Core library sources - manually specify critical files
set(NEO_CORE_SOURCES
    # Core IO
    src/io/uint160.cpp
    src/io/uint256.cpp
    src/io/binary_reader.cpp
    src/io/binary_writer.cpp
    src/io/byte_span.cpp
    src/io/json_reader.cpp
    src/io/json_writer.cpp
    
    # Extensions
    src/extensions/utility.cpp
    src/extensions/string_extensions.cpp
    src/extensions/collection_extensions.cpp
    src/extensions/datetime_extensions.cpp
    
    # Protocol
    src/protocol_settings.cpp
    src/hardfork.cpp
    
    # Cryptography basics
    src/cryptography/hash.cpp
    src/cryptography/crypto.cpp
    src/cryptography/ecc.cpp
    
    # Ledger basics
    src/ledger/block_header.cpp
    src/ledger/block.cpp
    src/ledger/blockchain.cpp
    src/ledger/signer.cpp
    src/ledger/witness.cpp
    
    # Network basics
    src/network/ip_endpoint.cpp
    src/network/message.cpp
    src/network/tcp_server.cpp
    src/network/tcp_client.cpp
    src/network/p2p_server.cpp
    
    # VM components
    src/vm/script_builder.cpp
    src/vm/script.cpp
    src/vm/stack_item.cpp
    src/vm/execution_engine.cpp
    
    # Node core
    src/node/neo_node.cpp
    src/node/neo_system.cpp
    
    # Persistence basics
    src/persistence/memory_store.cpp
    src/persistence/data_cache.cpp
    src/persistence/storage_key.cpp
    src/persistence/storage_item.cpp
    
    # Console service
    src/console_service/console_service_base.cpp
    src/console_service/console_command_method.cpp
    
    # Logging
    src/logging/logger.cpp
)

# Create core library
add_library(neo-core STATIC ${NEO_CORE_SOURCES})

# Link system libraries
target_link_libraries(neo-core
    PUBLIC
        Threads::Threads
        ${CMAKE_DL_LIBS}
)

if(OpenSSL_FOUND)
    target_link_libraries(neo-core PUBLIC OpenSSL::SSL OpenSSL::Crypto)
    target_compile_definitions(neo-core PUBLIC HAVE_OPENSSL)
endif()

# Set target properties
set_target_properties(neo-core PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
)

# Node executable
add_executable(neo-node src/node/main.cpp)
target_link_libraries(neo-node PRIVATE neo-core)

# Set executable properties
set_target_properties(neo-node PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Installation
install(TARGETS neo-node neo-core
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Print configuration summary
message(STATUS "")
message(STATUS "Neo C++ Configuration Summary:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "")
message(STATUS "Configuration complete. Run 'make' to build.")
message(STATUS "")