/**
 * @file jump_table_extension.cpp
 * @brief Jump Table Extension
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/vm/compound_items.h>
#include <neo/vm/exceptions.h>
#include <neo/vm/execution_engine.h>
#include <neo/vm/jump_table.h>
#include <neo/vm/jump_table_extension.h>
#include <neo/vm/primitive_items.h>
#include <neo/vm/special_items.h>
#include <neo/vm/stack_item.h>

namespace neo::vm
{
// Extension operations

void JumpTableExtension::ABORTMSG(ExecutionEngine& engine, const Instruction& /* instruction */)
{
    auto message = engine.Pop()->GetString();
    throw InvalidOperationException("ABORT: " + message);
}

void JumpTableExtension::ASSERTMSG(ExecutionEngine& engine, const Instruction& /* instruction */)
{
    auto message = engine.Pop()->GetString();
    auto condition = engine.Pop()->GetBoolean();

    if (!condition) throw InvalidOperationException("ASSERT: " + message);
}

void JumpTableExtension::PUSHT(ExecutionEngine& engine, const Instruction& /* instruction */)
{
    engine.Push(StackItem::Create(true));
}

void JumpTableExtension::PUSHF(ExecutionEngine& engine, const Instruction& /* instruction */)
{
    engine.Push(StackItem::Create(false));
}

void JumpTableExtension::CALLT(ExecutionEngine& engine, const Instruction& instruction)
{
    auto index = instruction.TokenU16();
    auto& context = engine.GetCurrentContext();
    auto table = context.GetCallTable();

    if (index >= table.size()) throw InvalidOperationException("Call table index out of range");

    auto position = table[index];
    JumpTable::ExecuteCall(engine, position);
}

// Type checking and conversion operations

void JumpTableExtension::ISNULL(ExecutionEngine& engine, const Instruction& /* instruction */)
{
    auto item = engine.Pop();
    engine.Push(StackItem::Create(item->IsNull()));
}

void JumpTableExtension::ISTYPE(ExecutionEngine& engine, const Instruction& instruction)
{
    auto type = static_cast<StackItemType>(instruction.TokenU8());
    auto item = engine.Pop();
    engine.Push(StackItem::Create(item->GetType() == type));
}

void JumpTableExtension::CONVERT(ExecutionEngine& engine, const Instruction& instruction)
{
    auto type = static_cast<StackItemType>(instruction.TokenU8());
    auto item = engine.Pop();
    engine.Push(item->ConvertTo(type));
}
}  // namespace neo::vm
