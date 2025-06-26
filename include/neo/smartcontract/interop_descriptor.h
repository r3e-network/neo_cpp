// Copyright (C) 2015-2025 The Neo Project.
//
// interop_descriptor.h file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#ifndef NEO_SMARTCONTRACT_INTEROP_DESCRIPTOR_H
#define NEO_SMARTCONTRACT_INTEROP_DESCRIPTOR_H

#include <string>
#include <vector>
#include <functional>
#include <cstdint>

namespace neo {
namespace smartcontract {

class ApplicationEngine;
class InteropParameterDescriptor;

/**
 * @brief Represents the call flags for interop services.
 */
enum class CallFlags : uint8_t {
    None = 0,
    ReadStates = 0b00000001,
    WriteStates = 0b00000010,
    AllowCall = 0b00000100,
    AllowNotify = 0b00001000,
    States = ReadStates | WriteStates,
    ReadOnly = ReadStates | AllowCall,
    All = States | AllowCall | AllowNotify
};

inline CallFlags operator|(CallFlags a, CallFlags b) {
    return static_cast<CallFlags>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline CallFlags operator&(CallFlags a, CallFlags b) {
    return static_cast<CallFlags>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

inline bool operator!=(CallFlags a, CallFlags b) {
    return static_cast<uint8_t>(a) != static_cast<uint8_t>(b);
}

/**
 * @brief Represents a descriptor for an interoperable service.
 */
struct InteropDescriptor {
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
    InteropDescriptor(std::string name, uint32_t hash, 
                     std::function<void(ApplicationEngine&)> handler,
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

} // namespace smartcontract
} // namespace neo

#endif // NEO_SMARTCONTRACT_INTEROP_DESCRIPTOR_H