# Include GoogleTest if available
find_package(GTest CONFIG)
if(GTest_FOUND)
    include(GoogleTest)
    enable_testing()

    # Make GoogleTest available to all tests
    set(GTEST_LIBRARIES GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main)
    message(STATUS "GTest found - unit tests will be built")
else()
    message(WARNING "GTest not found - unit tests will be skipped")
endif()

# Add subdirectories only if GTest is found
if(GTest_FOUND)
    add_subdirectory(unit)
    add_subdirectory(integration)
    add_subdirectory(plugins)
endif()

# Add network compatibility test utility
add_executable(network_compatibility network_compatibility.cpp)
target_link_libraries(network_compatibility PRIVATE neo_cpp)
target_include_directories(network_compatibility PRIVATE ${CMAKE_SOURCE_DIR}/include)
