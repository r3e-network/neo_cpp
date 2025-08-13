# Code Coverage Configuration for Neo C++
# 
# This module enables code coverage reporting using gcov/lcov
# Usage: cmake -DENABLE_COVERAGE=ON ..

option(ENABLE_COVERAGE "Enable code coverage reporting" OFF)

if(ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(STATUS "Code coverage enabled")
        
        # Add required flags
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
        
        # Find required tools
        find_program(GCOV_EXECUTABLE gcov)
        find_program(LCOV_EXECUTABLE lcov)
        find_program(GENHTML_EXECUTABLE genhtml)
        
        if(NOT GCOV_EXECUTABLE)
            message(WARNING "gcov not found - coverage reports will not be available")
        endif()
        
        if(NOT LCOV_EXECUTABLE)
            message(WARNING "lcov not found - HTML coverage reports will not be available")
        endif()
        
        if(NOT GENHTML_EXECUTABLE)
            message(WARNING "genhtml not found - HTML coverage reports will not be available")
        endif()
        
        # Add custom target for coverage report generation
        if(LCOV_EXECUTABLE AND GENHTML_EXECUTABLE)
            add_custom_target(coverage
                COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/coverage
                COMMAND ${LCOV_EXECUTABLE} --capture --initial --directory . --output-file ${CMAKE_BINARY_DIR}/coverage/baseline.info
                COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
                COMMAND ${LCOV_EXECUTABLE} --capture --directory . --output-file ${CMAKE_BINARY_DIR}/coverage/test.info
                COMMAND ${LCOV_EXECUTABLE} --add-tracefile ${CMAKE_BINARY_DIR}/coverage/baseline.info --add-tracefile ${CMAKE_BINARY_DIR}/coverage/test.info --output-file ${CMAKE_BINARY_DIR}/coverage/total.info
                COMMAND ${LCOV_EXECUTABLE} --remove ${CMAKE_BINARY_DIR}/coverage/total.info '/usr/*' '*/third_party/*' '*/tests/*' '*/build/*' --output-file ${CMAKE_BINARY_DIR}/coverage/filtered.info
                COMMAND ${GENHTML_EXECUTABLE} ${CMAKE_BINARY_DIR}/coverage/filtered.info --output-directory ${CMAKE_BINARY_DIR}/coverage/html --title "Neo C++ Code Coverage" --legend --show-details --demangle-cpp
                COMMAND ${CMAKE_COMMAND} -E echo "Coverage report generated at: ${CMAKE_BINARY_DIR}/coverage/html/index.html"
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT "Generating code coverage report..."
            )
            
            add_custom_target(coverage-clean
                COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/coverage
                COMMAND find ${CMAKE_BINARY_DIR} -name "*.gcda" -delete
                COMMAND find ${CMAKE_BINARY_DIR} -name "*.gcno" -delete
                COMMAND ${CMAKE_COMMAND} -E echo "Coverage data cleaned"
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT "Cleaning coverage data..."
            )
        endif()
        
    else()
        message(WARNING "Code coverage is only supported with GCC or Clang compilers")
        set(ENABLE_COVERAGE OFF)
    endif()
endif()

# Function to add coverage to a specific target
function(target_enable_coverage target)
    if(ENABLE_COVERAGE)
        target_compile_options(${target} PRIVATE --coverage -fprofile-arcs -ftest-coverage)
        target_link_options(${target} PRIVATE --coverage)
    endif()
endfunction()

# Macro to generate coverage report for specific test
macro(add_coverage_test test_name test_executable)
    if(ENABLE_COVERAGE AND LCOV_EXECUTABLE AND GENHTML_EXECUTABLE)
        add_custom_target(coverage-${test_name}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/coverage/${test_name}
            COMMAND ${LCOV_EXECUTABLE} --zerocounters --directory .
            COMMAND $<TARGET_FILE:${test_executable}>
            COMMAND ${LCOV_EXECUTABLE} --capture --directory . --output-file ${CMAKE_BINARY_DIR}/coverage/${test_name}/coverage.info
            COMMAND ${LCOV_EXECUTABLE} --remove ${CMAKE_BINARY_DIR}/coverage/${test_name}/coverage.info '/usr/*' '*/third_party/*' '*/tests/*' --output-file ${CMAKE_BINARY_DIR}/coverage/${test_name}/filtered.info
            COMMAND ${GENHTML_EXECUTABLE} ${CMAKE_BINARY_DIR}/coverage/${test_name}/filtered.info --output-directory ${CMAKE_BINARY_DIR}/coverage/${test_name}/html --title "${test_name} Coverage"
            COMMAND ${CMAKE_COMMAND} -E echo "Coverage report for ${test_name}: ${CMAKE_BINARY_DIR}/coverage/${test_name}/html/index.html"
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            DEPENDS ${test_executable}
            COMMENT "Generating coverage report for ${test_name}..."
        )
    endif()
endmacro()