/**
 * @file jump_table_type.cpp
 * @brief Jump Table Type
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/vm/compound_items.h>
#include <neo/vm/exceptions.h>
#include <neo/vm/execution_engine.h>
#include <neo/vm/jump_table.h>
#include <neo/vm/jump_table_type.h>
#include <neo/vm/primitive_items.h>
#include <neo/vm/special_items.h>
#include <neo/vm/stack_item.h>

namespace neo::vm
{
// JumpTable delegates to JumpTableType
void JumpTable::ISNULL(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableType::ISNULL(engine, instruction);
}

void JumpTable::ISTYPE(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableType::ISTYPE(engine, instruction);
}

void JumpTable::CONVERT(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableType::CONVERT(engine, instruction);
}

// JumpTableType implementations
void JumpTableType::ISNULL(ExecutionEngine& engine, const Instruction& instruction)
{
    (void)instruction;  // Suppress unused parameter warning
    auto item = engine.Pop();
    bool isNull = item->IsNull();
    engine.Push(StackItem::Create(isNull));
}

void JumpTableType::ISTYPE(ExecutionEngine& engine, const Instruction& instruction)
{
    auto type = static_cast<StackItemType>(instruction.TokenU8());
    auto item = engine.Pop();
    bool isType = item->GetType() == type;
    engine.Push(StackItem::Create(isType));
}

void JumpTableType::CONVERT(ExecutionEngine& engine, const Instruction& instruction)
{
    auto type = static_cast<StackItemType>(instruction.TokenU8());
    auto item = engine.Pop();

    switch (type)
    {
        case StackItemType::Boolean:
            engine.Push(StackItem::Create(item->GetBoolean()));
            break;
        case StackItemType::Integer:
            engine.Push(StackItem::Create(item->GetInteger()));
            break;
        case StackItemType::ByteString:
            engine.Push(StackItem::Create(item->GetByteArray()));
            break;
        case StackItemType::Buffer:
            engine.Push(std::static_pointer_cast<StackItem>(std::make_shared<BufferItem>(item->GetByteArray())));
            break;
        case StackItemType::Array:
        {
            if (item->GetType() == StackItemType::Array)
            {
                engine.Push(item);
            }
            else if (item->GetType() == StackItemType::Struct)
            {
                auto structItem = std::dynamic_pointer_cast<StructItem>(item);
                auto array = std::make_shared<ArrayItem>(structItem->GetArray(), engine.GetReferenceCounter());
                engine.Push(std::static_pointer_cast<StackItem>(array));
            }
            else
            {
                throw InvalidOperationException("Cannot convert this type to Array");
            }
            break;
        }
        case StackItemType::Struct:
        {
            if (item->GetType() == StackItemType::Struct)
            {
                engine.Push(item);
            }
            else if (item->GetType() == StackItemType::Array)
            {
                auto arrayItem = std::dynamic_pointer_cast<ArrayItem>(item);
                auto structItem = std::make_shared<StructItem>(arrayItem->GetArray(), engine.GetReferenceCounter());
                engine.Push(std::static_pointer_cast<StackItem>(structItem));
            }
            else
            {
                throw InvalidOperationException("Cannot convert this type to Struct");
            }
            break;
        }
        case StackItemType::Map:
        {
            if (item->GetType() == StackItemType::Map)
            {
                engine.Push(item);
            }
            else
            {
                throw InvalidOperationException("Cannot convert this type to Map");
            }
            break;
        }
        default:
            throw InvalidOperationException("Invalid conversion type");
    }
}
}  // namespace neo::vm
