/**
 * @file fuzz_vm_execution.cpp
 * @brief Fuzzing harness for Neo VM execution engine
 * Uses libFuzzer to find crashes and undefined behavior in VM
 */

#include <neo/vm/execution_engine.h>
#include <neo/vm/script_builder.h>
#include <neo/vm/opcode.h>
#include <neo/io/byte_vector.h>
#include <cstdint>
#include <cstddef>
#include <vector>
#include <algorithm>

using namespace neo::vm;
using namespace neo::io;

// Fuzzer entry point for libFuzzer
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    // Minimum size check
    if (size < 1 || size > 65536) {
        return 0;
    }
    
    try {
        // Create VM instance
        ExecutionEngine vm;
        
        // Load fuzzed script
        ByteVector script(data, data + size);
        vm.LoadScript(script);
        
        // Set resource limits to prevent infinite loops
        vm.SetGasLimit(10000000); // 0.1 GAS limit
        vm.SetMaxStackSize(2048);
        vm.SetMaxInvocationStackSize(1024);
        
        // Execute with timeout
        auto start = std::chrono::steady_clock::now();
        const auto timeout = std::chrono::seconds(1);
        
        while (vm.GetState() == VMState::None || vm.GetState() == VMState::Break) {
            // Check timeout
            if (std::chrono::steady_clock::now() - start > timeout) {
                break;
            }
            
            // Step execution
            vm.StepInto();
            
            // Check for excessive resource usage
            if (vm.GetGasConsumed() > vm.GetGasLimit()) {
                break;
            }
        }
        
        // Check final state (Halt or Fault are both valid outcomes)
        auto state = vm.GetState();
        if (state != VMState::Halt && state != VMState::Fault) {
            // Unexpected state - might be interesting
            return 0;
        }
        
    } catch (const std::exception& e) {
        // Exceptions are expected for invalid scripts
        // But we want to make sure we don't crash
        return 0;
    } catch (...) {
        // Catch any other exceptions
        return 0;
    }
    
    return 0;
}

// Seed corpus generator for better fuzzing
extern "C" size_t LLVMFuzzerCustomMutator(uint8_t* data, size_t size,
                                          size_t max_size, unsigned int seed) {
    if (size == 0 || max_size < 2) {
        return 0;
    }
    
    std::mt19937 rng(seed);
    std::uniform_int_distribution<> dist(0, 255);
    
    // Mutation strategies
    enum MutationType {
        INSERT_OPCODE,
        MODIFY_OPCODE,
        DELETE_BYTE,
        SWAP_BYTES,
        INSERT_PUSH,
        INSERT_VALID_SEQUENCE
    };
    
    std::uniform_int_distribution<> mutation_dist(0, 5);
    MutationType mutation = static_cast<MutationType>(mutation_dist(rng));
    
    std::vector<uint8_t> mutated(data, data + size);
    
    switch (mutation) {
        case INSERT_OPCODE: {
            // Insert a random valid opcode
            if (mutated.size() < max_size) {
                std::uniform_int_distribution<> pos_dist(0, mutated.size());
                std::uniform_int_distribution<> op_dist(0x00, 0xFF);
                size_t pos = pos_dist(rng);
                uint8_t opcode = op_dist(rng);
                mutated.insert(mutated.begin() + pos, opcode);
            }
            break;
        }
        
        case MODIFY_OPCODE: {
            // Modify an existing byte to a valid opcode
            if (!mutated.empty()) {
                std::uniform_int_distribution<> pos_dist(0, mutated.size() - 1);
                std::uniform_int_distribution<> op_dist(0x00, 0xFF);
                mutated[pos_dist(rng)] = op_dist(rng);
            }
            break;
        }
        
        case DELETE_BYTE: {
            // Delete a random byte
            if (mutated.size() > 1) {
                std::uniform_int_distribution<> pos_dist(0, mutated.size() - 1);
                mutated.erase(mutated.begin() + pos_dist(rng));
            }
            break;
        }
        
        case SWAP_BYTES: {
            // Swap two random bytes
            if (mutated.size() > 1) {
                std::uniform_int_distribution<> pos_dist(0, mutated.size() - 1);
                size_t pos1 = pos_dist(rng);
                size_t pos2 = pos_dist(rng);
                std::swap(mutated[pos1], mutated[pos2]);
            }
            break;
        }
        
        case INSERT_PUSH: {
            // Insert a valid PUSH sequence
            if (mutated.size() + 2 < max_size) {
                std::uniform_int_distribution<> pos_dist(0, mutated.size());
                std::uniform_int_distribution<> val_dist(0, 127);
                size_t pos = pos_dist(rng);
                uint8_t value = val_dist(rng);
                
                // Insert PUSH1 followed by value
                mutated.insert(mutated.begin() + pos, static_cast<uint8_t>(OpCode::PUSH1));
                mutated.insert(mutated.begin() + pos + 1, value);
            }
            break;
        }
        
        case INSERT_VALID_SEQUENCE: {
            // Insert a known valid opcode sequence
            const std::vector<std::vector<uint8_t>> valid_sequences = {
                // PUSH + ADD
                {0x11, 0x01, 0x11, 0x02, 0x93},
                // DUP + SWAP
                {0x76, 0x7C},
                // IF + ENDIF
                {0x13, 0x00, 0x01, 0x15},
                // TRY + ENDTRY
                {0x3A, 0x00, 0x01, 0x3B},
                // CALL + RET
                {0x34, 0x00, 0x01, 0x40}
            };
            
            if (!valid_sequences.empty() && mutated.size() < max_size - 5) {
                std::uniform_int_distribution<> seq_dist(0, valid_sequences.size() - 1);
                std::uniform_int_distribution<> pos_dist(0, mutated.size());
                
                const auto& seq = valid_sequences[seq_dist(rng)];
                size_t pos = pos_dist(rng);
                
                if (mutated.size() + seq.size() <= max_size) {
                    mutated.insert(mutated.begin() + pos, seq.begin(), seq.end());
                }
            }
            break;
        }
    }
    
    // Copy mutated data back
    size_t new_size = std::min(mutated.size(), max_size);
    std::copy(mutated.begin(), mutated.begin() + new_size, data);
    
    return new_size;
}

// Initialize fuzzer with seed corpus
extern "C" int LLVMFuzzerInitialize(int* argc, char*** argv) {
    // Set up any global state needed for fuzzing
    return 0;
}