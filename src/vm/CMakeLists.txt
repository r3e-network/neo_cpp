# Create the VM library
add_library(neo_vm OBJECT
    stack_item.cpp
    primitive_items.cpp
    compound_items.cpp
    special_items.cpp
    script.cpp
    script_builder.cpp
    execution_context.cpp
    exception_handling_context.cpp
    system_call.cpp
    execution_engine.cpp
    execution_engine_limits.cpp
    instruction.cpp
    opcode.cpp
    jump_table.cpp
    jump_table_constants.cpp
    jump_table_arithmetic.cpp
    jump_table_arithmetic_static.cpp
    jump_table_arithmetic_numeric.cpp
    jump_table_arithmetic_bitwise.cpp
    jump_table_stack.cpp
    jump_table_stack_static.cpp
    jump_table_compound.cpp
    jump_table_compound_static.cpp
    jump_table_compound_array.cpp
    jump_table_compound_map.cpp
    jump_table_control.cpp
    jump_table_control_jump.cpp
    jump_table_control_exception.cpp
    jump_table_slot.cpp
    jump_table_slot_static.cpp
    jump_table_slot_local.cpp
    jump_table_slot_argument.cpp
    jump_table_splice.cpp
    jump_table_splice_buffer.cpp
    jump_table_splice_string.cpp
    jump_table_extension.cpp
    jump_table_type.cpp
    jump_table_exception.cpp
    reference_counter.cpp
    debugger.cpp
)

# Include directories
target_include_directories(neo_vm
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# No external dependencies
# The VM module is now self-contained
