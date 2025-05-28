#pragma once

#include <neo/vm/jump_table.h>
#include <neo/vm/jump_table_splice_buffer.h>
#include <neo/vm/jump_table_splice_string.h>

namespace neo::vm
{
    /**
     * @brief Splice-related opcode handlers for the JumpTable.
     *
     * This class is a facade that delegates to the specialized splice operation classes:
     * - JumpTableSpliceBuffer: Buffer operations
     * - JumpTableSpliceString: String operations
     */
    class JumpTableSplice : public JumpTableSpliceBuffer, public JumpTableSpliceString
    {
    };
}
