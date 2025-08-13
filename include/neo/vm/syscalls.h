/**
 * @file syscalls.h
 * @brief Syscalls
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <cstdint>
#include <functional>

namespace neo::vm
{

class ExecutionEngine;

/**
 * @brief Register a system call handler
 * @param id The syscall identifier (usually a hash of the method name)
 * @param handler The function to handle the syscall
 */
void RegisterSyscall(uint32_t id, std::function<void(ExecutionEngine&)> handler);

/**
 * @brief Handle a system call
 * @param engine The execution engine making the syscall
 * @param id The syscall identifier
 * @return true if the syscall was handled successfully, false otherwise
 */
bool HandleSyscall(ExecutionEngine& engine, uint32_t id);

}  // namespace neo::vm