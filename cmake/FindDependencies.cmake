# Find and setup dependencies for Neo C++

# Find required packages
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# Try to find vcpkg packages first, then system packages
find_package(nlohmann_json CONFIG QUIET)
if(NOT nlohmann_json_FOUND)
    # Use the bundled nlohmann/json header
    add_library(nlohmann_json::nlohmann_json INTERFACE IMPORTED)
    target_include_directories(nlohmann_json::nlohmann_json INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    message(STATUS "Using bundled nlohmann/json")
endif()

find_package(spdlog CONFIG QUIET)
if(NOT spdlog_FOUND)
    # Create a minimal logging interface
    add_library(spdlog::spdlog INTERFACE IMPORTED)
    target_compile_definitions(spdlog::spdlog INTERFACE NEO_MINIMAL_LOGGING)
    message(STATUS "spdlog not found - using minimal logging")
else()
    # Ensure consistent macro across all translation units
    add_compile_definitions(NEO_HAS_SPDLOG)
    add_compile_definitions(HAS_SPDLOG)
    message(STATUS "Found spdlog - using full logging capabilities")
endif()

find_package(GTest CONFIG QUIET)
if(NOT GTest_FOUND)
    message(STATUS "Google Test not found - tests will be disabled")
    set(GTEST_LIBRARIES "")
else()
    set(GTEST_LIBRARIES GTest::gtest GTest::gtest_main GTest::gmock)
endif()

find_package(leveldb CONFIG QUIET)
if(NOT leveldb_FOUND)
    message(STATUS "LevelDB not found - using memory storage only")
    set(LEVELDB_LIBRARIES "")
else()
    set(LEVELDB_LIBRARIES leveldb::leveldb)
endif()

find_package(RocksDB CONFIG QUIET)
if(NOT RocksDB_FOUND)
    message(STATUS "RocksDB not found - using memory storage only")
    set(ROCKSDB_LIBRARIES "")
else()
    set(ROCKSDB_LIBRARIES RocksDB::rocksdb)
endif()

# Boost is optional
find_package(Boost QUIET COMPONENTS system filesystem thread)
if(Boost_FOUND)
    message(STATUS "Found Boost: ${Boost_VERSION}")
    set(BOOST_LIBRARIES Boost::system Boost::filesystem Boost::thread)
else()
    message(STATUS "Boost not found - some features will be disabled")
    set(BOOST_LIBRARIES "")
endif()

# httplib for RPC server
find_package(httplib CONFIG QUIET)
if(NOT httplib_FOUND)
    # Try to find httplib.h in third_party directory
    find_path(HTTPLIB_INCLUDE_DIR httplib.h 
        PATHS ${CMAKE_SOURCE_DIR}/third_party/httplib
        NO_DEFAULT_PATH)
    if(HTTPLIB_INCLUDE_DIR)
        message(STATUS "Found httplib header at ${HTTPLIB_INCLUDE_DIR}")
        set(HTTPLIB_FOUND TRUE)
        include_directories(${HTTPLIB_INCLUDE_DIR})
        add_compile_definitions(HAS_HTTPLIB)
        set(HTTPLIB_LIBRARIES "")
    else()
        message(STATUS "httplib not found - RPC server will be disabled")
        set(HTTPLIB_LIBRARIES "")
    endif()
else()
    message(STATUS "Found httplib - RPC server enabled")
    set(HTTPLIB_LIBRARIES httplib::httplib)
    add_compile_definitions(HAS_HTTPLIB)
endif()

find_package(ZLIB REQUIRED)

# Common libraries
set(NEO_COMMON_LIBRARIES
    ${CMAKE_THREAD_LIBS_INIT}
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    ${LEVELDB_LIBRARIES}
    ${ROCKSDB_LIBRARIES}
    ${BOOST_LIBRARIES}
    ${HTTPLIB_LIBRARIES}
    ZLIB::ZLIB
)
