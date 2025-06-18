#include <neo/vm/jump_table.h>
#include <neo/vm/execution_engine.h>
#include <neo/vm/stack_item.h>
#include <neo/vm/exceptions.h>
#include <neo/vm/primitive_items.h>
#include <neo/vm/compound_items.h>
#include <neo/vm/special_items.h>

namespace neo::vm
{
    // Stack operations

    void JumpTable::DEPTH(ExecutionEngine& engine, const Instruction&)
    {
        auto& context = engine.GetCurrentContext();
        auto size = context.GetEvaluationStack().size();
        engine.Push(StackItem::Create(static_cast<int64_t>(size)));
    }

    void JumpTable::DROP(ExecutionEngine& engine, const Instruction&)
    {
        engine.Pop();
    }

    void JumpTable::NIP(ExecutionEngine& engine, const Instruction&)
    {
        auto x = engine.Pop();
        engine.Pop();
        engine.Push(x);
    }

    void JumpTable::XDROP(ExecutionEngine& engine, const Instruction&)
    {
        auto n = engine.Pop()->GetInteger();
        if (n < 0)
            throw InvalidOperationException("Negative index for XDROP");

        auto& context = engine.GetCurrentContext();
        auto& stack = context.GetEvaluationStack();

        if (n >= static_cast<int64_t>(stack.size()))
            throw InvalidOperationException("Index out of range for XDROP");

        std::vector<std::shared_ptr<StackItem>> items;
        for (int64_t i = 0; i < n; i++)
        {
            items.push_back(engine.Pop());
        }

        engine.Pop(); // Drop the item at index n

        for (auto it = items.rbegin(); it != items.rend(); ++it)
        {
            engine.Push(*it);
        }
    }

    void JumpTable::CLEAR(ExecutionEngine& engine, const Instruction&)
    {
        auto& context = engine.GetCurrentContext();
        context.ClearStack();
    }

    void JumpTable::DUP(ExecutionEngine& engine, const Instruction&)
    {
        auto x = engine.Peek(0);
        engine.Push(x);
    }

    void JumpTable::OVER(ExecutionEngine& engine, const Instruction&)
    {
        auto x = engine.Peek(1);
        engine.Push(x);
    }

    void JumpTable::PICK(ExecutionEngine& engine, const Instruction&)
    {
        auto n = engine.Pop()->GetInteger();
        if (n < 0)
            throw InvalidOperationException("Negative index for PICK");

        auto x = engine.Peek(static_cast<int32_t>(n));
        engine.Push(x);
    }

    void JumpTable::TUCK(ExecutionEngine& engine, const Instruction&)
    {
        auto x1 = engine.Pop();
        auto x2 = engine.Pop();
        engine.Push(x1);
        engine.Push(x2);
        engine.Push(x1);
    }

    void JumpTable::SWAP(ExecutionEngine& engine, const Instruction&)
    {
        auto x1 = engine.Pop();
        auto x2 = engine.Pop();
        engine.Push(x1);
        engine.Push(x2);
    }

    void JumpTable::ROT(ExecutionEngine& engine, const Instruction&)
    {
        auto x1 = engine.Pop();
        auto x2 = engine.Pop();
        auto x3 = engine.Pop();
        engine.Push(x1);
        engine.Push(x3);
        engine.Push(x2);
    }

    void JumpTable::ROLL(ExecutionEngine& engine, const Instruction&)
    {
        auto n = engine.Pop()->GetInteger();
        if (n < 0)
            throw InvalidOperationException("Negative index for ROLL");

        if (n == 0)
            return;

        auto& context = engine.GetCurrentContext();
        auto& stack = context.GetEvaluationStack();

        if (n >= static_cast<int64_t>(stack.size()))
            throw InvalidOperationException("Index out of range for ROLL");

        std::vector<std::shared_ptr<StackItem>> items;
        for (int64_t i = 0; i < n; i++)
        {
            items.push_back(engine.Pop());
        }

        auto target = engine.Pop(); // The item to roll to the top

        for (auto it = items.rbegin(); it != items.rend(); ++it)
        {
            engine.Push(*it);
        }

        engine.Push(target);
    }

    void JumpTable::REVERSE3(ExecutionEngine& engine, const Instruction&)
    {
        auto x1 = engine.Pop();
        auto x2 = engine.Pop();
        auto x3 = engine.Pop();
        engine.Push(x1);
        engine.Push(x2);
        engine.Push(x3);
    }

    void JumpTable::REVERSE4(ExecutionEngine& engine, const Instruction&)
    {
        auto x1 = engine.Pop();
        auto x2 = engine.Pop();
        auto x3 = engine.Pop();
        auto x4 = engine.Pop();
        engine.Push(x1);
        engine.Push(x2);
        engine.Push(x3);
        engine.Push(x4);
    }

    void JumpTable::REVERSEN(ExecutionEngine& engine, const Instruction&)
    {
        auto n = engine.Pop()->GetInteger();
        if (n < 0)
            throw InvalidOperationException("Negative count for REVERSEN");

        if (n <= 1)
            return;

        auto& context = engine.GetCurrentContext();
        auto& stack = context.GetEvaluationStack();

        if (n > static_cast<int64_t>(stack.size()))
            throw InvalidOperationException("Count out of range for REVERSEN");

        std::vector<std::shared_ptr<StackItem>> items;
        for (int64_t i = 0; i < n; i++)
        {
            items.push_back(engine.Pop());
        }

        for (const auto& item : items)
        {
            engine.Push(item);
        }
    }
}
