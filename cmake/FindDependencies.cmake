# Find and setup dependencies for Neo C++

# Find required packages
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# Try to find vcpkg packages
find_package(nlohmann_json CONFIG)
find_package(GTest CONFIG)
find_package(leveldb CONFIG)

# Setup nlohmann/json
if(nlohmann_json_FOUND)
    message(STATUS "Found nlohmann/json via vcpkg")
    set(JSON_LIBRARIES nlohmann_json::nlohmann_json)
else()
    message(STATUS "nlohmann/json not found via vcpkg, using fallback")
    set(JSON_LIBRARIES "")
endif()

# Setup Google Test
if(GTest_FOUND)
    message(STATUS "Found Google Test via vcpkg")
    set(GTEST_LIBRARIES GTest::gtest GTest::gtest_main GTest::gmock)
else()
    message(STATUS "Google Test not found via vcpkg, using fallback")
    set(GTEST_LIBRARIES "")
endif()

# Setup LevelDB
if(leveldb_FOUND)
    message(STATUS "Found LevelDB via vcpkg")
    set(LEVELDB_LIBRARIES leveldb::leveldb)
else()
    message(STATUS "LevelDB not found via vcpkg, using fallback")
    set(LEVELDB_LIBRARIES "")
endif()

# Common libraries
set(NEO_COMMON_LIBRARIES
    ${CMAKE_THREAD_LIBS_INIT}
    OpenSSL::SSL
    OpenSSL::Crypto
    ${JSON_LIBRARIES}
    ${LEVELDB_LIBRARIES}
)