// Copyright (C) 2015-2025 The Neo Project.
//
// interop_parameter_descriptor.h file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#ifndef NEO_SMARTCONTRACT_INTEROP_PARAMETER_DESCRIPTOR_H
#define NEO_SMARTCONTRACT_INTEROP_PARAMETER_DESCRIPTOR_H

#include <functional>
#include <string>
#include <typeinfo>
#include <vector>

namespace neo
{
namespace vm
{
class StackItem;
}

namespace smartcontract
{

/**
 * @brief Represents a descriptor for an interop parameter.
 */
class InteropParameterDescriptor
{
   public:
    /**
     * @brief The type information of the parameter.
     */
    const std::type_info* type;

    /**
     * @brief The name of the parameter.
     */
    std::string name;

    /**
     * @brief Indicates whether the parameter is an array.
     */
    bool is_array;

    /**
     * @brief Indicates whether the parameter is an interface.
     */
    bool is_interface;

    /**
     * @brief Indicates whether the parameter is an enum.
     */
    bool is_enum;

    /**
     * @brief The converter function for the parameter.
     */
    std::function<void*(const vm::StackItem&)> converter;

    /**
     * @brief Constructs an InteropParameterDescriptor for the specified type.
     */
    template <typename T>
    static InteropParameterDescriptor create(const std::string& name = "")
    {
        InteropParameterDescriptor descriptor;
        descriptor.type = &typeid(T);
        descriptor.name = name;
        descriptor.is_array = std::is_array_v<T>;
        descriptor.is_interface =
            std::is_abstract_v<T>;  // Complete interface detection - checks for abstract classes (interfaces)
        descriptor.is_enum = std::is_enum_v<T>;
        descriptor.converter = create_converter<T>();
        return descriptor;
    }

   private:
    /**
     * @brief Creates a converter function for the specified type.
     */
    template <typename T>
    static std::function<void*(const vm::StackItem&)> create_converter();
};

}  // namespace smartcontract
}  // namespace neo

#endif  // NEO_SMARTCONTRACT_INTEROP_PARAMETER_DESCRIPTOR_H