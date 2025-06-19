# Testing setup for Neo C++

# Enable testing if Google Test is available
if(GTest_FOUND OR GTEST_LIBRARIES)
    enable_testing()
    
    # Function to create test targets
    function(neo_add_test target_name source_files)
        add_executable(${target_name} ${source_files})
        
        target_include_directories(${target_name}
            PRIVATE
                ${CMAKE_SOURCE_DIR}/include
                ${CMAKE_CURRENT_SOURCE_DIR}
        )
        
        target_link_libraries(${target_name}
            PRIVATE
                neo_cpp
                ${GTEST_LIBRARIES}
                ${NEO_COMMON_LIBRARIES}
        )
        
        # Add the test to CTest
        add_test(NAME ${target_name} COMMAND ${target_name})
        
        # Set test properties
        set_tests_properties(${target_name} PROPERTIES
            TIMEOUT 300
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        )
    endfunction()
    
    message(STATUS "Testing enabled - Google Test found")
else()
    message(WARNING "Testing disabled - Google Test not found")
    
    # Stub function when testing is not available
    function(neo_add_test target_name source_files)
        message(STATUS "Skipping test ${target_name} - Google Test not available")
    endfunction()
endif()