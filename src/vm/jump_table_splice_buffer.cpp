#include <neo/vm/compound_items.h>
#include <neo/vm/exceptions.h>
#include <neo/vm/execution_engine.h>
#include <neo/vm/jump_table.h>
#include <neo/vm/jump_table_splice.h>
#include <neo/vm/jump_table_splice_buffer.h>
#include <neo/vm/primitive_items.h>
#include <neo/vm/special_items.h>
#include <neo/vm/stack_item.h>

namespace neo::vm
{
// JumpTable delegates to JumpTableSpliceBuffer
void JumpTable::NEWBUFFER(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableSpliceBuffer::NEWBUFFER(engine, instruction);
}

void JumpTable::MEMCPY(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableSpliceBuffer::MEMCPY(engine, instruction);
}

// JumpTableSpliceBuffer implementations
void JumpTableSpliceBuffer::NEWBUFFER(ExecutionEngine& engine, const Instruction& /* instruction */)
{
    auto size = engine.Pop()->GetInteger();
    if (size < 0 || size > engine.GetLimits().MaxItemSize)
        throw InvalidOperationException("Invalid buffer size");

    io::ByteVector buffer(static_cast<size_t>(size));
    std::memset(buffer.Data(), 0, static_cast<size_t>(size));
    engine.Push(std::make_shared<BufferItem>(buffer));
}

void JumpTableSpliceBuffer::MEMCPY(ExecutionEngine& engine, const Instruction& /* instruction */)
{
    auto count = engine.Pop()->GetInteger();
    auto srcIndex = engine.Pop()->GetInteger();
    auto src = engine.Pop();
    auto dstIndex = engine.Pop()->GetInteger();
    auto dst = engine.Pop();

    if (count < 0)
        throw InvalidOperationException("Count cannot be negative");

    if (srcIndex < 0)
        throw InvalidOperationException("Source index cannot be negative");

    if (dstIndex < 0)
        throw InvalidOperationException("Destination index cannot be negative");

    if (dst->GetType() != StackItemType::Buffer)
        throw InvalidOperationException("Destination is not a buffer");

    auto srcBytes = src->GetByteArray();
    auto dstBytes = dst->GetByteArray();

    if (srcIndex + count > static_cast<int64_t>(srcBytes.Size()))
        throw InvalidOperationException("Source range exceeds buffer size");

    if (dstIndex + count > static_cast<int64_t>(dstBytes.Size()))
        throw InvalidOperationException("Destination range exceeds buffer size");

    // Create a new buffer with the copied data
    io::ByteVector newBuffer = dstBytes;
    for (int64_t i = 0; i < count; i++)
    {
        newBuffer[static_cast<size_t>(dstIndex + i)] = srcBytes[static_cast<size_t>(srcIndex + i)];
    }

    // Replace the buffer in the destination
    if (dst->GetType() == StackItemType::Buffer)
    {
        auto bufferItem = std::dynamic_pointer_cast<BufferItem>(dst);
        // Since BufferItem is immutable, we need to create a new one
        engine.Pop();  // Remove the old buffer
        engine.Push(std::make_shared<BufferItem>(newBuffer));
    }
}
}  // namespace neo::vm
