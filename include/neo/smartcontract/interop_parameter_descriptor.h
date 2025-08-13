/**
 * @file interop_parameter_descriptor.h
 * @brief Interop Parameter Descriptor
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

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