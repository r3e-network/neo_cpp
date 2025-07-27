#pragma once

// This file exists for backward compatibility
// It now includes the new split header files

#include <neo/vm/jump_table_control_exception.h>
#include <neo/vm/jump_table_control_jump.h>

namespace neo::vm
{
// Alias JumpTableControlJump and JumpTableControlException as JumpTableControl for backward compatibility
class JumpTableControl : public JumpTableControlJump, public JumpTableControlException
{
};
}  // namespace neo::vm
