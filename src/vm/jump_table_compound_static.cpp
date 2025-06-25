#include <neo/vm/jump_table_compound.h>
#include <neo/vm/jump_table_compound_array.h>
#include <neo/vm/jump_table_compound_map.h>
#include <neo/vm/execution_engine.h>
#include <neo/vm/stack_item.h>
#include <neo/vm/exceptions.h>
#include <neo/vm/primitive_items.h>
#include <neo/vm/compound_items.h>
#include <neo/vm/special_items.h>

namespace neo::vm
{
    // Static implementations for JumpTableCompound - these delegate to specialized classes
    
    void JumpTableCompound::PACK(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableCompoundArray::PACK(engine, instruction);
    }

    void JumpTableCompound::UNPACK(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableCompoundArray::UNPACK(engine, instruction);
    }

    void JumpTableCompound::NEWARRAY0(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableCompoundArray::NEWARRAY0(engine, instruction);
    }

    void JumpTableCompound::NEWARRAY(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableCompoundArray::NEWARRAY(engine, instruction);
    }

    void JumpTableCompound::NEWARRAY_T(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableCompoundArray::NEWARRAY_T(engine, instruction);
    }

    void JumpTableCompound::NEWSTRUCT0(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableCompoundArray::NEWSTRUCT0(engine, instruction);
    }

    void JumpTableCompound::NEWSTRUCT(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableCompoundArray::NEWSTRUCT(engine, instruction);
    }

    void JumpTableCompound::NEWMAP(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableCompoundMap::NEWMAP(engine, instruction);
    }

    void JumpTableCompound::SIZE(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableCompoundArray::SIZE(engine, instruction);
    }

    void JumpTableCompound::HASKEY(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableCompoundArray::HASKEY(engine, instruction);
    }

    void JumpTableCompound::KEYS(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableCompoundArray::KEYS(engine, instruction);
    }

    void JumpTableCompound::VALUES(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableCompoundArray::VALUES(engine, instruction);
    }

    void JumpTableCompound::PICKITEM(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableCompoundArray::PICKITEM(engine, instruction);
    }

    void JumpTableCompound::APPEND(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableCompoundArray::APPEND(engine, instruction);
    }

    void JumpTableCompound::SETITEM(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableCompoundArray::SETITEM(engine, instruction);
    }

    void JumpTableCompound::REMOVE(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableCompoundArray::REMOVE(engine, instruction);
    }

    void JumpTableCompound::CLEARITEMS(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableCompoundArray::CLEARITEMS(engine, instruction);
    }

    void JumpTableCompound::REVERSEITEMS(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableCompoundArray::REVERSEITEMS(engine, instruction);
    }

    void JumpTableCompound::POPITEM(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableCompoundArray::POPITEM(engine, instruction);
    }

    void JumpTableCompound::PACKMAP(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableCompoundMap::PACKMAP(engine, instruction);
    }

    void JumpTableCompound::PACKSTRUCT(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableCompoundArray::PACKSTRUCT(engine, instruction);
    }

    void JumpTableCompound::ISNULL(ExecutionEngine& engine, const Instruction& instruction)
    {
        auto item = engine.Pop();
        bool isNull = (item->GetType() == StackItemType::Null);
        engine.Push(StackItem::Create(isNull));
    }

    void JumpTableCompound::ISTYPE(ExecutionEngine& engine, const Instruction& instruction)
    {
        auto type = static_cast<StackItemType>(instruction.TokenU8());
        auto item = engine.Pop();
        bool isType = (item->GetType() == type);
        engine.Push(StackItem::Create(isType));
    }

    void JumpTableCompound::CONVERT(ExecutionEngine& engine, const Instruction& instruction)
    {
        auto type = static_cast<StackItemType>(instruction.TokenU8());
        auto item = engine.Pop();
        
        // Convert the item to the specified type
        std::shared_ptr<StackItem> convertedItem;
        
        switch (type)
        {
            case StackItemType::Boolean:
                convertedItem = StackItem::Create(item->GetBoolean());
                break;
            case StackItemType::Integer:
                convertedItem = StackItem::Create(item->GetInteger());
                break;
            case StackItemType::ByteString:
                convertedItem = StackItem::Create(item->GetByteArray());
                break;
            default:
                throw InvalidOperationException("Cannot convert to this type");
        }
        
        engine.Push(convertedItem);
    }
}