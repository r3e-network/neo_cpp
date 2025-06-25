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
)