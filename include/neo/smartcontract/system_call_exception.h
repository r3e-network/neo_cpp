/**
 * @file system_call_exception.h
 * @brief System Call Exception
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <stdexcept>
#include <string>

namespace neo::smartcontract
{
/**
 * @brief Exception thrown when a system call fails.
 *
 * This exception is thrown when a system call fails for any reason.
 * It provides information about the system call that failed and the reason for the failure.
 */
class SystemCallException : public std::runtime_error
{
   public:
    /**
     * @brief Constructs a SystemCallException.
     * @param systemCall The name of the system call that failed.
     * @param message The error message.
     */
    SystemCallException(const std::string& systemCall, const std::string& message)
        : std::runtime_error(FormatMessage(systemCall, message)), systemCall_(systemCall), message_(message)
    {
    }

    /**
     * @brief Gets the name of the system call that failed.
     * @return The name of the system call.
     */
    const std::string& GetSystemCall() const { return systemCall_; }

    /**
     * @brief Gets the error message.
     * @return The error message.
     */
    const std::string& GetMessage() const { return message_; }

   private:
    std::string systemCall_;
    std::string message_;

    /**
     * @brief Formats the error message.
     * @param systemCall The name of the system call that failed.
     * @param message The error message.
     * @return The formatted error message.
     */
    static std::string FormatMessage(const std::string& systemCall, const std::string& message)
    {
        return "System call '" + systemCall + "' failed: " + message;
    }
};

/**
 * @brief Exception thrown when a system call is invoked with invalid arguments.
 */
class InvalidArgumentException : public SystemCallException
{
   public:
    /**
     * @brief Constructs an InvalidArgumentException.
     * @param systemCall The name of the system call that failed.
     * @param message The error message.
     */
    InvalidArgumentException(const std::string& systemCall, const std::string& message)
        : SystemCallException(systemCall, message)
    {
    }
};

/**
 * @brief Exception thrown when a system call is invoked without the required flags.
 */
class MissingFlagsException : public SystemCallException
{
   public:
    /**
     * @brief Constructs a MissingFlagsException.
     * @param systemCall The name of the system call that failed.
     * @param requiredFlags The flags that are required.
     */
    MissingFlagsException(const std::string& systemCall, const std::string& requiredFlags)
        : SystemCallException(systemCall, "Missing required flags: " + requiredFlags)
    {
    }
};

/**
 * @brief Exception thrown when a system call is invoked with insufficient gas.
 */
class InsufficientGasException : public SystemCallException
{
   public:
    /**
     * @brief Constructs an InsufficientGasException.
     * @param systemCall The name of the system call that failed.
     * @param requiredGas The amount of gas required.
     * @param availableGas The amount of gas available.
     */
    InsufficientGasException(const std::string& systemCall, int64_t requiredGas, int64_t availableGas)
        : SystemCallException(systemCall, "Insufficient gas: required " + std::to_string(requiredGas) + ", available " +
                                              std::to_string(availableGas)),
          requiredGas_(requiredGas),
          availableGas_(availableGas)
    {
    }

    /**
     * @brief Gets the amount of gas required.
     * @return The amount of gas required.
     */
    int64_t GetRequiredGas() const { return requiredGas_; }

    /**
     * @brief Gets the amount of gas available.
     * @return The amount of gas available.
     */
    int64_t GetAvailableGas() const { return availableGas_; }

   private:
    int64_t requiredGas_;
    int64_t availableGas_;
};

/**
 * @brief Exception thrown when a system call is invoked with an invalid contract.
 */
class ContractNotFoundException : public SystemCallException
{
   public:
    /**
     * @brief Constructs a ContractNotFoundException.
     * @param systemCall The name of the system call that failed.
     * @param contractHash The hash of the contract that was not found.
     */
    ContractNotFoundException(const std::string& systemCall, const std::string& contractHash)
        : SystemCallException(systemCall, "Contract not found: " + contractHash)
    {
    }
};

/**
 * @brief Exception thrown when a system call is invoked with an invalid method.
 */
class MethodNotFoundException : public SystemCallException
{
   public:
    /**
     * @brief Constructs a MethodNotFoundException.
     * @param systemCall The name of the system call that failed.
     * @param contractHash The hash of the contract.
     * @param method The name of the method that was not found.
     */
    MethodNotFoundException(const std::string& systemCall, const std::string& contractHash, const std::string& method)
        : SystemCallException(systemCall, "Method '" + method + "' not found in contract " + contractHash)
    {
    }
};
}  // namespace neo::smartcontract
