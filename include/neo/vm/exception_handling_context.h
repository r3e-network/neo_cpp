#pragma once

#include <cstdint>
#include <memory>
#include <neo/vm/stack_item.h>

namespace neo::vm
{
/**
 * @brief Represents the state of exception handling.
 */
enum class ExceptionHandlingState
{
    /**
     * @brief The try block.
     */
    Try,

    /**
     * @brief The catch block.
     */
    Catch,

    /**
     * @brief The finally block.
     */
    Finally
};

/**
 * @brief Represents a context for exception handling.
 */
class ExceptionHandlingContext
{
  public:
    /**
     * @brief Constructs a new ExceptionHandlingContext.
     * @param catchPointer The catch pointer.
     * @param finallyPointer The finally pointer.
     */
    ExceptionHandlingContext(int32_t catchPointer, int32_t finallyPointer);

    /**
     * @brief Gets the catch pointer.
     * @return The catch pointer.
     */
    int32_t GetCatchPointer() const;

    /**
     * @brief Gets the finally pointer.
     * @return The finally pointer.
     */
    int32_t GetFinallyPointer() const;

    /**
     * @brief Gets the end pointer.
     * @return The end pointer.
     */
    int32_t GetEndPointer() const;

    /**
     * @brief Sets the end pointer.
     * @param endPointer The end pointer.
     */
    void SetEndPointer(int32_t endPointer);

    /**
     * @brief Gets the state.
     * @return The state.
     */
    ExceptionHandlingState GetState() const;

    /**
     * @brief Sets the state.
     * @param state The state.
     */
    void SetState(ExceptionHandlingState state);

    /**
     * @brief Checks if the context has a catch block.
     * @return True if the context has a catch block, false otherwise.
     */
    bool HasCatch() const;

    /**
     * @brief Checks if the context has a finally block.
     * @return True if the context has a finally block, false otherwise.
     */
    bool HasFinally() const;

  private:
    int32_t catchPointer_;
    int32_t finallyPointer_;
    int32_t endPointer_;
    ExceptionHandlingState state_;
};
}  // namespace neo::vm
