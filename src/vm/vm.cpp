/**
 * @file vm.cpp
 * @brief Main VM implementation entry point
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/vm/vm.h>
#include <neo/vm/execution_engine.h>
#include <neo/vm/script.h>
#include <neo/vm/stack_item.h>
#include <neo/vm/opcode.h>
#include <neo/core/exceptions.h>

#include <memory>
#include <vector>
#include <sstream>

namespace neo::vm
{

/**
 * @class VirtualMachine
 * @brief High-level VM interface for Neo blockchain
 */
class VirtualMachine::Impl
{
public:
    std::unique_ptr<ExecutionEngine> engine;
    std::vector<std::shared_ptr<Script>> loadedScripts;
    
    Impl() : engine(std::make_unique<ExecutionEngine>()) {}
};

VirtualMachine::VirtualMachine()
    : pImpl(std::make_unique<Impl>())
{
}

VirtualMachine::~VirtualMachine() = default;

bool VirtualMachine::LoadScript(const std::vector<uint8_t>& script)
{
    try 
    {
        auto scriptObj = std::make_shared<Script>(script);
        pImpl->loadedScripts.push_back(scriptObj);
        pImpl->engine->LoadScript(scriptObj);
        return true;
    }
    catch (const std::exception& e)
    {
        return false;
    }
}

bool VirtualMachine::Execute()
{
    if (!pImpl->engine)
    {
        return false;
    }
    
    try
    {
        pImpl->engine->Execute();
        return pImpl->engine->State() == VMState::HALT;
    }
    catch (const std::exception& e)
    {
        return false;
    }
}

VMState VirtualMachine::GetState() const
{
    if (!pImpl->engine)
    {
        return VMState::FAULT;
    }
    return pImpl->engine->State();
}

std::shared_ptr<StackItem> VirtualMachine::GetResult() const
{
    if (!pImpl->engine || pImpl->engine->ResultStack().Count() == 0)
    {
        return nullptr;
    }
    return pImpl->engine->ResultStack().Peek();
}

void VirtualMachine::Reset()
{
    pImpl->engine = std::make_unique<ExecutionEngine>();
    pImpl->loadedScripts.clear();
}

uint64_t VirtualMachine::GetGasConsumed() const
{
    if (!pImpl->engine)
    {
        return 0;
    }
    return pImpl->engine->GasConsumed();
}

void VirtualMachine::SetGasLimit(uint64_t limit)
{
    if (pImpl->engine)
    {
        // Set gas limit on the execution engine
        // Note: ExecutionEngine should have a SetGasLimit method
    }
}

bool VirtualMachine::LoadContext(const ExecutionContext& context)
{
    try
    {
        // Load the context into the engine
        // This would involve setting up the initial state
        return true;
    }
    catch (const std::exception& e)
    {
        return false;
    }
}

std::string VirtualMachine::GetErrorMessage() const
{
    if (!pImpl->engine || pImpl->engine->State() != VMState::FAULT)
    {
        return "";
    }
    
    // Return the fault message from the engine
    return "VM execution fault";
}

// Factory method
std::unique_ptr<VirtualMachine> VirtualMachine::Create()
{
    return std::make_unique<VirtualMachine>();
}

// Utility functions
bool VirtualMachine::VerifyScript(const std::vector<uint8_t>& script)
{
    try
    {
        Script s(script);
        // Basic verification: check for invalid opcodes
        for (size_t i = 0; i < script.size(); )
        {
            OpCode opcode = static_cast<OpCode>(script[i]);
            
            // Check if opcode is valid
            if (opcode > OpCode::ENDFINALLY)
            {
                return false;
            }
            
            // Skip operands based on opcode
            switch (opcode)
            {
                case OpCode::PUSHDATA1:
                    if (i + 1 >= script.size()) return false;
                    i += 2 + script[i + 1];
                    break;
                case OpCode::PUSHDATA2:
                    if (i + 2 >= script.size()) return false;
                    i += 3 + (script[i + 1] | (script[i + 2] << 8));
                    break;
                case OpCode::PUSHDATA4:
                    if (i + 4 >= script.size()) return false;
                    i += 5 + (script[i + 1] | (script[i + 2] << 8) | 
                             (script[i + 3] << 16) | (script[i + 4] << 24));
                    break;
                default:
                    i++;
                    break;
            }
        }
        return true;
    }
    catch (const std::exception& e)
    {
        return false;
    }
}

std::vector<uint8_t> VirtualMachine::SerializeStackItem(const StackItem& item)
{
    std::vector<uint8_t> result;
    
    // Serialize based on type
    switch (item.GetType())
    {
        case StackItemType::Integer:
        {
            auto value = item.GetInteger();
            // Serialize integer
            result.push_back(static_cast<uint8_t>(StackItemType::Integer));
            // Add integer bytes (little-endian)
            for (int i = 0; i < 8; ++i)
            {
                result.push_back(static_cast<uint8_t>(value >> (i * 8)));
            }
            break;
        }
        case StackItemType::ByteString:
        {
            auto bytes = item.GetByteArray();
            result.push_back(static_cast<uint8_t>(StackItemType::ByteString));
            // Add length (as 4 bytes, little-endian)
            uint32_t len = static_cast<uint32_t>(bytes.size());
            for (int i = 0; i < 4; ++i)
            {
                result.push_back(static_cast<uint8_t>(len >> (i * 8)));
            }
            // Add bytes
            result.insert(result.end(), bytes.begin(), bytes.end());
            break;
        }
        case StackItemType::Boolean:
        {
            result.push_back(static_cast<uint8_t>(StackItemType::Boolean));
            result.push_back(item.GetBoolean() ? 1 : 0);
            break;
        }
        default:
            // Handle other types
            result.push_back(static_cast<uint8_t>(StackItemType::Any));
            break;
    }
    
    return result;
}

std::shared_ptr<StackItem> VirtualMachine::DeserializeStackItem(const std::vector<uint8_t>& data)
{
    if (data.empty())
    {
        return nullptr;
    }
    
    StackItemType type = static_cast<StackItemType>(data[0]);
    size_t offset = 1;
    
    switch (type)
    {
        case StackItemType::Integer:
        {
            if (data.size() < 9) return nullptr;
            int64_t value = 0;
            for (int i = 0; i < 8; ++i)
            {
                value |= static_cast<int64_t>(data[offset + i]) << (i * 8);
            }
            return StackItem::CreateInteger(value);
        }
        case StackItemType::ByteString:
        {
            if (data.size() < 5) return nullptr;
            uint32_t len = 0;
            for (int i = 0; i < 4; ++i)
            {
                len |= static_cast<uint32_t>(data[offset + i]) << (i * 8);
            }
            offset += 4;
            if (data.size() < offset + len) return nullptr;
            std::vector<uint8_t> bytes(data.begin() + offset, data.begin() + offset + len);
            return StackItem::CreateByteString(bytes);
        }
        case StackItemType::Boolean:
        {
            if (data.size() < 2) return nullptr;
            return StackItem::CreateBoolean(data[offset] != 0);
        }
        default:
            return StackItem::CreateNull();
    }
}

} // namespace neo::vm