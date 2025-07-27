#pragma once

#include <stdexcept>
#include <string>

namespace neo::vm
{
/**
 * @brief Base class for all VM exceptions.
 */
class VMException : public std::runtime_error
{
  public:
    /**
     * @brief Constructs a new VMException.
     * @param message The error message.
     */
    explicit VMException(const std::string& message) : std::runtime_error(message) {}
};

/**
 * @brief Exception thrown when a script is invalid.
 */
class BadScriptException : public VMException
{
  public:
    /**
     * @brief Constructs a new BadScriptException with the specified message.
     * @param message The error message.
     */
    explicit BadScriptException(const std::string& message) : VMException(message) {}
};

/**
 * @brief Exception thrown when a VM operation is invalid.
 */
class InvalidOperationException : public VMException
{
  public:
    /**
     * @brief Constructs a new InvalidOperationException with the specified message.
     * @param message The error message.
     */
    explicit InvalidOperationException(const std::string& message) : VMException(message) {}
};

/**
 * @brief Exception thrown when a stack overflow occurs.
 */
class StackOverflowException : public VMException
{
  public:
    /**
     * @brief Constructs a new StackOverflowException with the specified message.
     * @param message The error message.
     */
    explicit StackOverflowException(const std::string& message) : VMException(message) {}
};

/**
 * @brief Exception thrown when a stack underflow occurs.
 */
class StackUnderflowException : public VMException
{
  public:
    /**
     * @brief Constructs a new StackUnderflowException with the specified message.
     * @param message The error message.
     */
    explicit StackUnderflowException(const std::string& message) : VMException(message) {}
};

/**
 * @brief Exception thrown when a type conversion is invalid.
 */
class InvalidCastException : public VMException
{
  public:
    /**
     * @brief Constructs a new InvalidCastException with the specified message.
     * @param message The error message.
     */
    explicit InvalidCastException(const std::string& message) : VMException(message) {}
};

/**
 * @brief Exception thrown when an operation is not supported.
 */
class NotSupportedException : public VMException
{
  public:
    /**
     * @brief Constructs a new NotSupportedException.
     * @param message The error message.
     */
    explicit NotSupportedException(const std::string& message) : VMException(message) {}
};

/**
 * @brief Exception thrown when an argument is invalid.
 */
class ArgumentException : public VMException
{
  public:
    /**
     * @brief Constructs a new ArgumentException.
     * @param message The error message.
     */
    explicit ArgumentException(const std::string& message) : VMException(message) {}
};

/**
 * @brief Exception thrown when an argument is out of range.
 */
class ArgumentOutOfRangeException : public ArgumentException
{
  public:
    /**
     * @brief Constructs a new ArgumentOutOfRangeException.
     * @param message The error message.
     */
    explicit ArgumentOutOfRangeException(const std::string& message) : ArgumentException(message) {}
};

/**
 * @brief Exception thrown when an argument is null.
 */
class ArgumentNullException : public ArgumentException
{
  public:
    /**
     * @brief Constructs a new ArgumentNullException.
     * @param message The error message.
     */
    explicit ArgumentNullException(const std::string& message) : ArgumentException(message) {}
};

/**
 * @brief Exception that can be caught by the VM.
 */
class CatchableException : public VMException
{
  public:
    /**
     * @brief Constructs a new CatchableException.
     * @param message The error message.
     */
    explicit CatchableException(const std::string& message) : VMException(message) {}
};

/**
 * @brief Exception thrown when a division by zero occurs.
 */
class DivideByZeroException : public VMException
{
  public:
    /**
     * @brief Constructs a new DivideByZeroException.
     * @param message The error message.
     */
    explicit DivideByZeroException(const std::string& message = "Division by zero") : VMException(message) {}
};
}  // namespace neo::vm
