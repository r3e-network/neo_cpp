/**
 * @file interop_descriptor.h
 * @brief Interop Descriptor
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#ifndef NEO_SMARTCONTRACT_INTEROP_DESCRIPTOR_H
#define NEO_SMARTCONTRACT_INTEROP_DESCRIPTOR_H

#include <neo/smartcontract/call_flags.h>

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace neo
{
namespace smartcontract
{

class ApplicationEngine;
class InteropParameterDescriptor;

/**
 * @brief Represents a descriptor for an interoperable service.
 */
struct InteropDescriptor
{
    /**
     * @brief The name of the interoperable service.
     */
    std::string name;

    /**
     * @brief The hash of the interoperable service.
     */
    uint32_t hash;

    /**
     * @brief The handler function for the interoperable service.
     */
    std::function<void(ApplicationEngine&)> handler;

    /**
     * @brief The fixed price of the interoperable service.
     */
    int64_t fixed_price;

    /**
     * @brief The required call flags for the interoperable service.
     */
    CallFlags required_call_flags;

    /**
     * @brief The parameters of the interoperable service.
     */
    std::vector<InteropParameterDescriptor> parameters;

    /**
     * @brief Constructs an InteropDescriptor.
     */
    InteropDescriptor() = default;

    /**
     * @brief Constructs an InteropDescriptor with the specified parameters.
     */
    InteropDescriptor(std::string name, uint32_t hash, std::function<void(ApplicationEngine&)> handler,
                      int64_t fixed_price, CallFlags required_call_flags);

    /**
     * @brief Implicit conversion to uint32_t (hash).
     */
    operator uint32_t() const { return hash; }
};

/**
 * @brief Calculates the hash of an interop service name.
 * @param name The name of the interop service
 * @return The hash of the name
 */
uint32_t calculate_interop_hash(const std::string& name);

}  // namespace smartcontract
}  // namespace neo

#endif  // NEO_SMARTCONTRACT_INTEROP_DESCRIPTOR_H