#include <neo/vm/jump_table.h>
#include <neo/vm/jump_table_splice.h>
#include <neo/vm/jump_table_splice_string.h>
#include <neo/vm/execution_engine.h>
#include <neo/vm/stack_item.h>
#include <neo/vm/exceptions.h>
#include <neo/vm/primitive_items.h>
#include <neo/vm/compound_items.h>
#include <neo/vm/special_items.h>
#include <cstring>

namespace neo::vm
{
    // JumpTable delegates to JumpTableSpliceString
    void JumpTable::CAT(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableSpliceString::CAT(engine, instruction);
    }

    void JumpTable::SUBSTR(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableSpliceString::SUBSTR(engine, instruction);
    }

    void JumpTable::LEFT(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableSpliceString::LEFT(engine, instruction);
    }

    void JumpTable::RIGHT(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableSpliceString::RIGHT(engine, instruction);
    }

    // JumpTableSpliceString implementations
    void JumpTableSpliceString::CAT(ExecutionEngine& engine, const Instruction& instruction)
    {
        auto x2 = engine.Pop()->GetByteArray();
        auto x1 = engine.Pop()->GetByteArray();

        size_t totalSize = x1.Size() + x2.Size();
        if (totalSize > engine.GetLimits().MaxItemSize)
            throw InvalidOperationException("Result is too large");

        io::ByteVector result(totalSize);
        std::memcpy(result.Data(), x1.Data(), x1.Size());
        std::memcpy(result.Data() + x1.Size(), x2.Data(), x2.Size());

        engine.Push(StackItem::Create(result));
    }

    void JumpTableSpliceString::SUBSTR(ExecutionEngine& engine, const Instruction& instruction)
    {
        auto length = engine.Pop()->GetInteger();
        auto index = engine.Pop()->GetInteger();
        auto x = engine.Pop()->GetByteArray();

        if (index < 0)
            throw InvalidOperationException("Index cannot be negative");

        if (length < 0)
            throw InvalidOperationException("Length cannot be negative");

        if (index + length > static_cast<int64_t>(x.Size()))
            throw InvalidOperationException("Range exceeds string length");

        io::ByteVector result(static_cast<size_t>(length));
        std::memcpy(result.Data(), x.Data() + index, static_cast<size_t>(length));

        engine.Push(StackItem::Create(result));
    }

    void JumpTableSpliceString::LEFT(ExecutionEngine& engine, const Instruction& instruction)
    {
        auto count = engine.Pop()->GetInteger();
        auto x = engine.Pop()->GetByteArray();

        if (count < 0)
            throw InvalidOperationException("Count cannot be negative");

        count = std::min(count, static_cast<int64_t>(x.Size()));

        io::ByteVector result(static_cast<size_t>(count));
        std::memcpy(result.Data(), x.Data(), static_cast<size_t>(count));

        engine.Push(StackItem::Create(result));
    }

    void JumpTableSpliceString::RIGHT(ExecutionEngine& engine, const Instruction& instruction)
    {
        auto count = engine.Pop()->GetInteger();
        auto x = engine.Pop()->GetByteArray();

        if (count < 0)
            throw InvalidOperationException("Count cannot be negative");

        count = std::min(count, static_cast<int64_t>(x.Size()));

        io::ByteVector result(static_cast<size_t>(count));
        std::memcpy(result.Data(), x.Data() + x.Size() - count, static_cast<size_t>(count));

        engine.Push(StackItem::Create(result));
    }
}
