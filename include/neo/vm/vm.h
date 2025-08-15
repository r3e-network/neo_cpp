/**
 * @file vm.h
 * @brief Main VM interface for Neo blockchain
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#ifndef NEO_VM_VM_H
#define NEO_VM_VM_H

#include <memory>
#include <vector>
#include <string>
#include <cstdint>

namespace neo::vm
{

// Forward declarations
class StackItem;
class ExecutionContext;

/**
 * @enum VMState
 * @brief Virtual machine execution states
 */
enum class VMState
{
    NONE,
    HALT,
    FAULT,
    BREAK
};

/**
 * @class VirtualMachine
 * @brief High-level VM interface for Neo blockchain
 * 
 * Provides methods for loading scripts, executing them, and retrieving results.
 */
class VirtualMachine
{
public:
    /**
     * @brief Constructor
     */
    VirtualMachine();
    
    /**
     * @brief Destructor
     */
    ~VirtualMachine();
    
    /**
     * @brief Load a script into the VM
     * @param script The script bytecode
     * @return true if loaded successfully, false otherwise
     */
    bool LoadScript(const std::vector<uint8_t>& script);
    
    /**
     * @brief Execute the loaded script
     * @return true if execution succeeded, false if faulted
     */
    bool Execute();
    
    /**
     * @brief Get the current VM state
     * @return The VM state
     */
    VMState GetState() const;
    
    /**
     * @brief Get the result from the result stack
     * @return The top item from the result stack, or nullptr if empty
     */
    std::shared_ptr<StackItem> GetResult() const;
    
    /**
     * @brief Reset the VM to initial state
     */
    void Reset();
    
    /**
     * @brief Get gas consumed during execution
     * @return The amount of gas consumed
     */
    uint64_t GetGasConsumed() const;
    
    /**
     * @brief Set the gas limit for execution
     * @param limit The gas limit
     */
    void SetGasLimit(uint64_t limit);
    
    /**
     * @brief Load an execution context
     * @param context The execution context
     * @return true if loaded successfully, false otherwise
     */
    bool LoadContext(const ExecutionContext& context);
    
    /**
     * @brief Get error message if VM is in fault state
     * @return Error message string, empty if not faulted
     */
    std::string GetErrorMessage() const;
    
    // Factory methods
    
    /**
     * @brief Create a new VM instance
     * @return A unique pointer to the VM
     */
    static std::unique_ptr<VirtualMachine> Create();
    
    // Utility functions
    
    /**
     * @brief Verify a script without executing it
     * @param script The script bytecode
     * @return true if script is valid, false otherwise
     */
    static bool VerifyScript(const std::vector<uint8_t>& script);
    
    /**
     * @brief Serialize a stack item to bytes
     * @param item The stack item
     * @return Serialized bytes
     */
    static std::vector<uint8_t> SerializeStackItem(const StackItem& item);
    
    /**
     * @brief Deserialize bytes to a stack item
     * @param data The serialized data
     * @return The deserialized stack item
     */
    static std::shared_ptr<StackItem> DeserializeStackItem(const std::vector<uint8_t>& data);
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace neo::vm

#endif // NEO_VM_VM_H