#include <neo/vm/jump_table.h>
#include <neo/vm/jump_table_compound.h>
#include <neo/vm/jump_table_compound_array.h>
#include <neo/vm/execution_engine.h>
#include <neo/vm/stack_item.h>
#include <neo/vm/exceptions.h>
#include <neo/vm/primitive_items.h>
#include <neo/vm/compound_items.h>
#include <neo/vm/special_items.h>

namespace neo::vm
{
    // JumpTable delegates to JumpTableCompoundArray
    void JumpTable::PACK(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableCompoundArray::PACK(engine, instruction);
    }

    void JumpTable::UNPACK(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableCompoundArray::UNPACK(engine, instruction);
    }

    void JumpTable::NEWARRAY0(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableCompoundArray::NEWARRAY0(engine, instruction);
    }

    void JumpTable::NEWARRAY(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableCompoundArray::NEWARRAY(engine, instruction);
    }

    void JumpTable::NEWARRAY_T(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableCompoundArray::NEWARRAY_T(engine, instruction);
    }

    void JumpTable::NEWSTRUCT0(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableCompoundArray::NEWSTRUCT0(engine, instruction);
    }

    void JumpTable::NEWSTRUCT(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableCompoundArray::NEWSTRUCT(engine, instruction);
    }

    void JumpTable::SIZE(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableCompoundArray::SIZE(engine, instruction);
    }

    void JumpTable::HASKEY(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableCompoundArray::HASKEY(engine, instruction);
    }

    void JumpTable::KEYS(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableCompoundArray::KEYS(engine, instruction);
    }

    void JumpTable::VALUES(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableCompoundArray::VALUES(engine, instruction);
    }

    void JumpTable::PICKITEM(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableCompoundArray::PICKITEM(engine, instruction);
    }

    void JumpTable::APPEND(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableCompoundArray::APPEND(engine, instruction);
    }

    void JumpTable::SETITEM(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableCompoundArray::SETITEM(engine, instruction);
    }

    void JumpTable::REMOVE(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableCompoundArray::REMOVE(engine, instruction);
    }

    void JumpTable::CLEARITEMS(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableCompoundArray::CLEARITEMS(engine, instruction);
    }

    void JumpTable::REVERSEITEMS(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableCompoundArray::REVERSEITEMS(engine, instruction);
    }

    void JumpTable::POPITEM(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableCompoundArray::POPITEM(engine, instruction);
    }

    void JumpTable::PACKSTRUCT(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableCompoundArray::PACKSTRUCT(engine, instruction);
    }

    // JumpTableCompoundArray implementations
    void JumpTableCompoundArray::PACK(ExecutionEngine& engine, const Instruction& instruction)
    {
        auto size = engine.Pop()->GetInteger();
        if (size < 0 || size > 0xFFFF) // Reasonable limit
            throw InvalidOperationException("Invalid array size");

        auto& context = engine.GetCurrentContext();
        auto& stack = context.GetEvaluationStack();

        if (size > static_cast<int64_t>(stack.size()))
            throw InvalidOperationException("Stack underflow during PACK operation");

        std::vector<std::shared_ptr<StackItem>> items;
        for (int64_t i = 0; i < size; i++)
        {
            items.insert(items.begin(), engine.Pop());
        }

        auto array = std::make_shared<ArrayItem>(items, engine.GetReferenceCounter());
        engine.Push(array);
    }

    void JumpTableCompoundArray::UNPACK(ExecutionEngine& engine, const Instruction& instruction)
    {
        auto item = engine.Pop();

        if (item->GetType() == StackItemType::Array || item->GetType() == StackItemType::Struct)
        {
            auto array = item->GetArray();
            for (auto it = array.rbegin(); it != array.rend(); ++it)
            {
                engine.Push(*it);
            }
            engine.Push(StackItem::Create(static_cast<int64_t>(array.size())));
        }
        else if (item->GetType() == StackItemType::Map)
        {
            auto map = item->GetMap();
            for (auto it = map.rbegin(); it != map.rend(); ++it)
            {
                engine.Push(it->second);
                engine.Push(it->first);
            }
            engine.Push(StackItem::Create(static_cast<int64_t>(map.size() * 2)));
        }
        else
        {
            throw InvalidOperationException("Cannot unpack this type");
        }
    }

    void JumpTableCompoundArray::NEWARRAY0(ExecutionEngine& engine, const Instruction& instruction)
    {
        auto array = std::make_shared<ArrayItem>(std::vector<std::shared_ptr<StackItem>>{}, engine.GetReferenceCounter());
        engine.Push(array);
    }

    void JumpTableCompoundArray::NEWARRAY(ExecutionEngine& engine, const Instruction& instruction)
    {
        auto size = engine.Pop()->GetInteger();
        if (size < 0 || size > 0xFFFF) // Reasonable limit
            throw InvalidOperationException("Invalid array size");

        std::vector<std::shared_ptr<StackItem>> items(size, StackItem::Null());
        auto array = std::make_shared<ArrayItem>(items, engine.GetReferenceCounter());
        engine.Push(array);
    }

    void JumpTableCompoundArray::NEWARRAY_T(ExecutionEngine& engine, const Instruction& instruction)
    {
        auto size = engine.Pop()->GetInteger();
        if (size < 0 || size > 0xFFFF) // Reasonable limit
            throw InvalidOperationException("Invalid array size");

        auto type = static_cast<StackItemType>(instruction.TokenU8());
        std::vector<std::shared_ptr<StackItem>> items;

        for (int64_t i = 0; i < size; i++)
        {
            switch (type)
            {
                case StackItemType::Boolean:
                    items.push_back(std::make_shared<BooleanItem>(false));
                    break;
                case StackItemType::Integer:
                    items.push_back(std::make_shared<IntegerItem>(0));
                    break;
                case StackItemType::ByteString:
                    items.push_back(std::make_shared<ByteStringItem>(io::ByteVector{}));
                    break;
                default:
                    items.push_back(StackItem::Null());
                    break;
            }
        }

        auto array = std::make_shared<ArrayItem>(items, engine.GetReferenceCounter());
        engine.Push(array);
    }

    void JumpTableCompoundArray::NEWSTRUCT0(ExecutionEngine& engine, const Instruction& instruction)
    {
        auto structItem = std::make_shared<StructItem>(std::vector<std::shared_ptr<StackItem>>{}, engine.GetReferenceCounter());
        engine.Push(structItem);
    }

    void JumpTableCompoundArray::NEWSTRUCT(ExecutionEngine& engine, const Instruction& instruction)
    {
        auto size = engine.Pop()->GetInteger();
        if (size < 0 || size > 0xFFFF) // Reasonable limit
            throw InvalidOperationException("Invalid struct size");

        std::vector<std::shared_ptr<StackItem>> items(size, StackItem::Null());
        auto structItem = std::make_shared<StructItem>(items, engine.GetReferenceCounter());
        engine.Push(structItem);
    }

    void JumpTableCompoundArray::SIZE(ExecutionEngine& engine, const Instruction& instruction)
    {
        auto item = engine.Pop();
        int64_t size = 0;

        switch (item->GetType())
        {
            case StackItemType::Array:
            case StackItemType::Struct:
                size = static_cast<int64_t>(item->GetArray().size());
                break;
            case StackItemType::Map:
                size = static_cast<int64_t>(item->GetMap().size());
                break;
            case StackItemType::ByteString:
            case StackItemType::Buffer:
                size = static_cast<int64_t>(item->GetByteArray().Size());
                break;
            default:
                throw InvalidOperationException("Item doesn't support size operation");
        }

        engine.Push(StackItem::Create(size));
    }

    void JumpTableCompoundArray::HASKEY(ExecutionEngine& engine, const Instruction& instruction)
    {
        auto key = engine.Pop();
        auto container = engine.Pop();
        bool hasKey = false;

        switch (container->GetType())
        {
            case StackItemType::Array:
            case StackItemType::Struct:
            {
                auto index = key->GetInteger();
                auto array = container->GetArray();
                hasKey = index >= 0 && index < static_cast<int64_t>(array.size());
                break;
            }
            case StackItemType::Map:
            {
                auto map = container->GetMap();
                for (const auto& [k, v] : map)
                {
                    if (k->Equals(*key))
                    {
                        hasKey = true;
                        break;
                    }
                }
                break;
            }
            default:
                throw InvalidOperationException("Item doesn't support haskey operation");
        }

        engine.Push(StackItem::Create(hasKey));
    }

    void JumpTableCompoundArray::KEYS(ExecutionEngine& engine, const Instruction& instruction)
    {
        auto map = engine.Pop();
        if (map->GetType() != StackItemType::Map)
            throw InvalidOperationException("Item is not a map");

        auto mapData = map->GetMap();
        std::vector<std::shared_ptr<StackItem>> keys;

        for (const auto& [k, v] : mapData)
        {
            keys.push_back(k);
        }

        auto array = std::make_shared<ArrayItem>(keys, engine.GetReferenceCounter());
        engine.Push(array);
    }

    void JumpTableCompoundArray::VALUES(ExecutionEngine& engine, const Instruction& instruction)
    {
        auto container = engine.Pop();
        std::vector<std::shared_ptr<StackItem>> values;

        switch (container->GetType())
        {
            case StackItemType::Array:
            case StackItemType::Struct:
                values = container->GetArray();
                break;
            case StackItemType::Map:
            {
                auto map = container->GetMap();
                for (const auto& [k, v] : map)
                {
                    values.push_back(v);
                }
                break;
            }
            default:
                throw InvalidOperationException("Item doesn't support values operation");
        }

        auto array = std::make_shared<ArrayItem>(values, engine.GetReferenceCounter());
        engine.Push(array);
    }

    void JumpTableCompoundArray::PICKITEM(ExecutionEngine& engine, const Instruction& instruction)
    {
        auto key = engine.Pop();
        auto container = engine.Pop();

        switch (container->GetType())
        {
            case StackItemType::Array:
            case StackItemType::Struct:
            {
                auto index = key->GetInteger();
                auto array = container->GetArray();
                if (index < 0 || index >= static_cast<int64_t>(array.size()))
                    throw InvalidOperationException("Index out of range");

                engine.Push(array[static_cast<size_t>(index)]);
                break;
            }
            case StackItemType::Map:
            {
                auto map = container->GetMap();
                bool found = false;

                for (const auto& [k, v] : map)
                {
                    if (k->Equals(*key))
                    {
                        engine.Push(v);
                        found = true;
                        break;
                    }
                }

                if (!found)
                    throw InvalidOperationException("Key not found in map");

                break;
            }
            default:
                throw InvalidOperationException("Item doesn't support pickitem operation");
        }
    }

    void JumpTableCompoundArray::APPEND(ExecutionEngine& engine, const Instruction& instruction)
    {
        auto newItem = engine.Pop();
        auto array = engine.Pop();

        if (array->GetType() != StackItemType::Array && array->GetType() != StackItemType::Struct)
            throw InvalidOperationException("Item is not an array or struct");

        if (array->GetType() == StackItemType::Array)
        {
            auto arrayItem = std::dynamic_pointer_cast<ArrayItem>(array);
            arrayItem->Add(newItem);
        }
        else
        {
            auto structItem = std::dynamic_pointer_cast<StructItem>(array);
            auto clonedItem = newItem;

            if (newItem->GetType() == StackItemType::Struct)
            {
                auto structNewItem = std::dynamic_pointer_cast<StructItem>(newItem);
                clonedItem = structNewItem->Clone();
            }

            std::dynamic_pointer_cast<ArrayItem>(structItem)->Add(clonedItem);
        }
    }

    void JumpTableCompoundArray::SETITEM(ExecutionEngine& engine, const Instruction& instruction)
    {
        auto value = engine.Pop();
        auto key = engine.Pop();
        auto container = engine.Pop();

        switch (container->GetType())
        {
            case StackItemType::Array:
            case StackItemType::Struct:
            {
                auto index = key->GetInteger();

                if (container->GetType() == StackItemType::Array)
                {
                    auto arrayItem = std::dynamic_pointer_cast<ArrayItem>(container);
                    if (index < 0 || index >= static_cast<int64_t>(arrayItem->Size()))
                        throw InvalidOperationException("Index out of range");

                    arrayItem->Set(static_cast<size_t>(index), value);
                }
                else
                {
                    auto structItem = std::dynamic_pointer_cast<StructItem>(container);
                    if (index < 0 || index >= static_cast<int64_t>(structItem->Size()))
                        throw InvalidOperationException("Index out of range");

                    auto clonedValue = value;
                    if (value->GetType() == StackItemType::Struct)
                    {
                        auto structValue = std::dynamic_pointer_cast<StructItem>(value);
                        clonedValue = structValue->Clone();
                    }

                    std::dynamic_pointer_cast<ArrayItem>(structItem)->Set(static_cast<size_t>(index), clonedValue);
                }
                break;
            }
            case StackItemType::Map:
            {
                auto mapItem = std::dynamic_pointer_cast<MapItem>(container);
                mapItem->Set(key, value);
                break;
            }
            default:
                throw InvalidOperationException("Item doesn't support setitem operation");
        }
    }

    void JumpTableCompoundArray::REMOVE(ExecutionEngine& engine, const Instruction& instruction)
    {
        auto key = engine.Pop();
        auto container = engine.Pop();

        switch (container->GetType())
        {
            case StackItemType::Array:
            case StackItemType::Struct:
            {
                auto index = key->GetInteger();

                if (container->GetType() == StackItemType::Array)
                {
                    auto arrayItem = std::dynamic_pointer_cast<ArrayItem>(container);
                    if (index < 0 || index >= static_cast<int64_t>(arrayItem->Size()))
                        throw InvalidOperationException("Index out of range");

                    arrayItem->Remove(static_cast<size_t>(index));
                }
                else
                {
                    auto structItem = std::dynamic_pointer_cast<StructItem>(container);
                    if (index < 0 || index >= static_cast<int64_t>(structItem->Size()))
                        throw InvalidOperationException("Index out of range");

                    std::dynamic_pointer_cast<ArrayItem>(structItem)->Remove(static_cast<size_t>(index));
                }
                break;
            }
            case StackItemType::Map:
            {
                auto mapItem = std::dynamic_pointer_cast<MapItem>(container);
                mapItem->Remove(key);
                break;
            }
            default:
                throw InvalidOperationException("Item doesn't support remove operation");
        }
    }

    void JumpTableCompoundArray::CLEARITEMS(ExecutionEngine& engine, const Instruction& instruction)
    {
        auto container = engine.Pop();

        switch (container->GetType())
        {
            case StackItemType::Array:
            case StackItemType::Struct:
            {
                if (container->GetType() == StackItemType::Array)
                {
                    auto arrayItem = std::dynamic_pointer_cast<ArrayItem>(container);
                    arrayItem->Clear();
                }
                else
                {
                    auto structItem = std::dynamic_pointer_cast<StructItem>(container);
                    std::dynamic_pointer_cast<ArrayItem>(structItem)->Clear();
                }
                break;
            }
            case StackItemType::Map:
            {
                auto mapItem = std::dynamic_pointer_cast<MapItem>(container);
                mapItem->Clear();
                break;
            }
            default:
                throw InvalidOperationException("Item doesn't support clearitems operation");
        }
    }

    void JumpTableCompoundArray::REVERSEITEMS(ExecutionEngine& engine, const Instruction& instruction)
    {
        auto container = engine.Pop();

        if (container->GetType() != StackItemType::Array && container->GetType() != StackItemType::Struct)
            throw InvalidOperationException("Item is not an array or struct");

        auto array = container->GetArray();
        std::vector<std::shared_ptr<StackItem>> reversedArray;
        reversedArray.reserve(array.size());

        for (auto it = array.rbegin(); it != array.rend(); ++it)
        {
            reversedArray.push_back(*it);
        }

        if (container->GetType() == StackItemType::Array)
        {
            auto arrayItem = std::dynamic_pointer_cast<ArrayItem>(container);
            arrayItem->Clear();
            for (const auto& item : reversedArray)
            {
                arrayItem->Add(item);
            }
        }
        else
        {
            auto structItem = std::dynamic_pointer_cast<StructItem>(container);
            std::dynamic_pointer_cast<ArrayItem>(structItem)->Clear();
            for (const auto& item : reversedArray)
            {
                std::dynamic_pointer_cast<ArrayItem>(structItem)->Add(item);
            }
        }
    }

    void JumpTableCompoundArray::POPITEM(ExecutionEngine& engine, const Instruction& instruction)
    {
        auto container = engine.Pop();

        if (container->GetType() != StackItemType::Array && container->GetType() != StackItemType::Struct)
            throw InvalidOperationException("Item is not an array or struct");

        auto array = container->GetArray();
        if (array.empty())
            throw InvalidOperationException("Array is empty");

        auto lastItem = array.back();

        if (container->GetType() == StackItemType::Array)
        {
            auto arrayItem = std::dynamic_pointer_cast<ArrayItem>(container);
            arrayItem->Remove(arrayItem->Size() - 1);
        }
        else
        {
            auto structItem = std::dynamic_pointer_cast<StructItem>(container);
            std::dynamic_pointer_cast<ArrayItem>(structItem)->Remove(std::dynamic_pointer_cast<ArrayItem>(structItem)->Size() - 1);
        }

        engine.Push(lastItem);
    }

    void JumpTableCompoundArray::PACKSTRUCT(ExecutionEngine& engine, const Instruction& instruction)
    {
        auto size = engine.Pop()->GetInteger();
        if (size < 0 || size > 0xFFFF) // Reasonable limit
            throw InvalidOperationException("Invalid struct size");

        auto& context = engine.GetCurrentContext();
        auto& stack = context.GetEvaluationStack();

        if (size > static_cast<int64_t>(stack.size()))
            throw InvalidOperationException("Stack underflow during PACKSTRUCT operation");

        std::vector<std::shared_ptr<StackItem>> items;
        for (int64_t i = 0; i < size; i++)
        {
            items.insert(items.begin(), engine.Pop());
        }

        auto structItem = std::make_shared<StructItem>(items, engine.GetReferenceCounter());
        engine.Push(structItem);
    }
}
