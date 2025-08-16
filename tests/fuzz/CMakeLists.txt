cmake_minimum_required(VERSION 3.10)
project(neo_fuzz_tests)

# Check for fuzzer support
if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(WARNING "Fuzz testing requires Clang compiler")
    return()
endif()

# Set fuzzing flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=fuzzer,address,undefined")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O1")

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/src)

# Find required libraries
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# Helper function to add fuzz targets
function(add_fuzz_test name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} 
        neo_core
        ${OPENSSL_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
        -fsanitize=fuzzer,address,undefined
    )
    
    # Create corpus directory
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/corpus/${name})
    
    # Add to CTest
    add_test(
        NAME ${name}_quick
        COMMAND ${name} -max_total_time=10 -max_len=1024 ${CMAKE_BINARY_DIR}/corpus/${name}
    )
endfunction()

# Add individual fuzz targets
add_fuzz_test(fuzz_vm_execution)
add_fuzz_test(fuzz_crypto)
add_fuzz_test(fuzz_serialization)
add_fuzz_test(fuzz_network)

# Create seed corpus directory
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/corpus)

# Add custom target to run all fuzzers
add_custom_target(fuzz
    COMMAND echo "Running fuzz tests..."
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/fuzz_vm_execution -max_total_time=30 ${CMAKE_BINARY_DIR}/corpus/fuzz_vm_execution
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/fuzz_crypto -max_total_time=30 ${CMAKE_BINARY_DIR}/corpus/fuzz_crypto
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/fuzz_serialization -max_total_time=30 ${CMAKE_BINARY_DIR}/corpus/fuzz_serialization
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/fuzz_network -max_total_time=30 ${CMAKE_BINARY_DIR}/corpus/fuzz_network
    DEPENDS fuzz_vm_execution fuzz_crypto fuzz_serialization fuzz_network
    COMMENT "Running all fuzz tests for 30 seconds each"
)

# Add custom target for continuous fuzzing
add_custom_target(fuzz_continuous
    COMMAND echo "Starting continuous fuzzing (press Ctrl+C to stop)..."
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/fuzz_vm_execution ${CMAKE_BINARY_DIR}/corpus/fuzz_vm_execution
    DEPENDS fuzz_vm_execution
    COMMENT "Running continuous VM fuzzing"
)

# Generate seed corpus for VM fuzzer
add_custom_command(
    TARGET fuzz_vm_execution POST_BUILD
    COMMAND echo "Generating seed corpus for VM fuzzer..."
    COMMAND mkdir -p ${CMAKE_BINARY_DIR}/corpus/fuzz_vm_execution
    COMMAND echo -ne "\\x00" > ${CMAKE_BINARY_DIR}/corpus/fuzz_vm_execution/push0
    COMMAND echo -ne "\\x51" > ${CMAKE_BINARY_DIR}/corpus/fuzz_vm_execution/push1
    COMMAND echo -ne "\\x51\\x52\\x93" > ${CMAKE_BINARY_DIR}/corpus/fuzz_vm_execution/add
    COMMAND echo -ne "\\x51\\x52\\x94" > ${CMAKE_BINARY_DIR}/corpus/fuzz_vm_execution/sub
    COMMAND echo -ne "\\x51\\x52\\x95" > ${CMAKE_BINARY_DIR}/corpus/fuzz_vm_execution/mul
    COMMAND echo -ne "\\x51\\x52\\x96" > ${CMAKE_BINARY_DIR}/corpus/fuzz_vm_execution/div
    COMMENT "Generated seed corpus for VM fuzzer"
)

# Generate seed corpus for crypto fuzzer
add_custom_command(
    TARGET fuzz_crypto POST_BUILD
    COMMAND echo "Generating seed corpus for crypto fuzzer..."
    COMMAND mkdir -p ${CMAKE_BINARY_DIR}/corpus/fuzz_crypto
    COMMAND echo -ne "test" > ${CMAKE_BINARY_DIR}/corpus/fuzz_crypto/small
    COMMAND dd if=/dev/urandom of=${CMAKE_BINARY_DIR}/corpus/fuzz_crypto/random32 bs=32 count=1 2>/dev/null || true
    COMMAND dd if=/dev/urandom of=${CMAKE_BINARY_DIR}/corpus/fuzz_crypto/random256 bs=256 count=1 2>/dev/null || true
    COMMENT "Generated seed corpus for crypto fuzzer"
)

# Installation rules
install(TARGETS 
    fuzz_vm_execution
    fuzz_crypto
    fuzz_serialization
    fuzz_network
    RUNTIME DESTINATION bin/fuzz
)

install(DIRECTORY ${CMAKE_BINARY_DIR}/corpus
    DESTINATION share/neo/fuzz
)