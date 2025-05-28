#include <neo/vm/jump_table.h>
#include <neo/vm/jump_table_compound.h>
#include <neo/vm/jump_table_compound_map.h>
#include <neo/vm/execution_engine.h>
#include <neo/vm/stack_item.h>
#include <neo/vm/exceptions.h>
#include <neo/vm/primitive_items.h>
#include <neo/vm/compound_items.h>
#include <neo/vm/special_items.h>

namespace neo::vm
{
    // JumpTable delegates to JumpTableCompoundMap
    void JumpTable::NEWMAP(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableCompoundMap::NEWMAP(engine, instruction);
    }

    void JumpTable::PACKMAP(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableCompoundMap::PACKMAP(engine, instruction);
    }

    // JumpTableCompoundMap implementations
    void JumpTableCompoundMap::NEWMAP(ExecutionEngine& engine, const Instruction& instruction)
    {
        auto map = std::make_shared<MapItem>(std::map<std::shared_ptr<StackItem>, std::shared_ptr<StackItem>>{}, engine.GetReferenceCounter());
        engine.Push(map);
    }

    void JumpTableCompoundMap::PACKMAP(ExecutionEngine& engine, const Instruction& instruction)
    {
        auto size = engine.Pop()->GetInteger();
        if (size < 0 || size > 0xFFFF) // Reasonable limit
            throw InvalidOperationException("Invalid map size");

        auto& context = engine.GetCurrentContext();
        auto& stack = context.GetEvaluationStack();

        if (size * 2 > static_cast<int64_t>(stack.size()))
            throw InvalidOperationException("Stack underflow during PACKMAP operation");

        std::map<std::shared_ptr<StackItem>, std::shared_ptr<StackItem>> items;
        for (int64_t i = 0; i < size; i++)
        {
            auto value = engine.Pop();
            auto key = engine.Pop();
            items[key] = value;
        }

        auto map = std::make_shared<MapItem>(items, engine.GetReferenceCounter());
        engine.Push(map);
    }
}
