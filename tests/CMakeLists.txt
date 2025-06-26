# Check if GoogleTest is available either from find_package or as a subdirectory
find_package(GTest CONFIG QUIET)
if(GTest_FOUND)
    include(GoogleTest)
    enable_testing()
    # Make GoogleTest available to all tests
    set(GTEST_LIBRARIES GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main)
    message(STATUS "GTest found via find_package - unit tests will be built")
elseif(TARGET gtest)
    # GoogleTest was added as a subdirectory
    include(GoogleTest)
    enable_testing()
    set(GTEST_LIBRARIES gtest gtest_main gmock gmock_main)
    message(STATUS "GTest found as subdirectory - unit tests will be built")
else()
    message(WARNING "GTest not found - unit tests will be skipped")
endif()

# Add subdirectories only if GTest is found
if(GTest_FOUND OR TARGET gtest)
    add_subdirectory(unit)
    add_subdirectory(integration)
    add_subdirectory(plugins)
endif()

# Add simple test to verify test infrastructure
if(GTest_FOUND OR TARGET gtest)
    add_executable(simple_unit_test simple_unit_test.cpp)
    target_link_libraries(simple_unit_test PRIVATE ${GTEST_LIBRARIES})
    add_test(NAME simple_unit_test COMMAND simple_unit_test)
endif()

# Add network compatibility test utility - disabled for now due to API changes
# add_executable(network_compatibility network_compatibility.cpp)
# target_link_libraries(network_compatibility PRIVATE neo_cpp)
# target_include_directories(network_compatibility PRIVATE ${CMAKE_SOURCE_DIR}/include)
