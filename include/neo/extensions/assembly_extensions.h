#pragma once

#include <functional>
#include <memory>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <vector>

namespace neo::extensions
{
/**
 * @brief Extensions for assembly and type reflection.
 *
 * ## Overview
 * Provides utilities for type information, method discovery, and runtime reflection
 * capabilities similar to .NET Assembly reflection but adapted for C++.
 *
 * ## API Reference
 * - **Type Information**: Get type names, sizes, and properties
 * - **Method Discovery**: Find and invoke methods by name
 * - **Utilities**: Type registration, factory patterns
 *
 * ## Usage Examples
 * ```cpp
 * // Get type information
 * auto info = AssemblyExtensions::GetTypeInfo<MyClass>();
 *
 * // Check if type is registered
 * bool exists = AssemblyExtensions::HasType("MyClass");
 *
 * // Create instance by name
 * auto instance = AssemblyExtensions::CreateInstance("MyClass");
 * ```
 *
 * ## Design Notes
 * - Uses RTTI where available for type information
 * - Provides factory pattern for object creation
 * - Thread-safe type registration system
 */
class AssemblyExtensions
{
  public:
    /**
     * @brief Type information structure
     */
    struct TypeInfo
    {
        std::string name;         ///< Type name
        std::string mangledName;  ///< Mangled type name from typeid
        size_t size;              ///< Size in bytes
        bool isPointer;           ///< Whether it's a pointer type
        bool isReference;         ///< Whether it's a reference type
        bool isConst;             ///< Whether it's const qualified

        TypeInfo() : size(0), isPointer(false), isReference(false), isConst(false) {}
    };

    /**
     * @brief Method information structure
     */
    struct MethodInfo
    {
        std::string name;                ///< Method name
        std::string signature;           ///< Method signature
        std::function<void*()> invoker;  ///< Method invoker function

        MethodInfo() = default;
        MethodInfo(const std::string& n, const std::string& sig) : name(n), signature(sig) {}
    };

    /**
     * @brief Factory function type for creating instances
     */
    using FactoryFunction = std::function<std::shared_ptr<void>()>;

    /**
     * @brief Get type information for a given type
     * @tparam T Type to get information for
     * @return TypeInfo structure with type details
     */
    template <typename T>
    static TypeInfo GetTypeInfo()
    {
        TypeInfo info;
        info.name = GetTypeName<T>();
        info.mangledName = typeid(T).name();
        info.size = sizeof(T);
        info.isPointer = std::is_pointer_v<T>;
        info.isReference = std::is_reference_v<T>;
        info.isConst = std::is_const_v<std::remove_reference_t<T>>;
        return info;
    }

    /**
     * @brief Get clean type name (without namespace prefixes)
     * @tparam T Type to get name for
     * @return Clean type name
     */
    template <typename T>
    static std::string GetTypeName()
    {
        std::string name = typeid(T).name();
        return DemangleTypeName(name);
    }

    /**
     * @brief Register a type with factory function
     * @tparam T Type to register
     * @param name Type name (optional, uses type name if empty)
     * @param factory Factory function to create instances
     */
    template <typename T>
    static void RegisterType(const std::string& name = "", FactoryFunction factory = nullptr)
    {
        std::string typeName = name.empty() ? GetTypeName<T>() : name;

        if (!factory)
        {
            factory = []() -> std::shared_ptr<void> { return std::make_shared<T>(); };
        }

        GetTypeRegistry()[typeName] = factory;
        GetTypeInfoRegistry()[typeName] = GetTypeInfo<T>();
    }

    /**
     * @brief Check if a type is registered
     * @param typeName Name of the type
     * @return True if type is registered
     */
    static bool HasType(const std::string& typeName);

    /**
     * @brief Create instance of registered type
     * @param typeName Name of the type
     * @return Shared pointer to created instance, or nullptr if not found
     */
    static std::shared_ptr<void> CreateInstance(const std::string& typeName);

    /**
     * @brief Get list of all registered type names
     * @return Vector of registered type names
     */
    static std::vector<std::string> GetRegisteredTypes();

    /**
     * @brief Get type information for registered type
     * @param typeName Name of the type
     * @return TypeInfo structure, or empty structure if not found
     */
    static TypeInfo GetRegisteredTypeInfo(const std::string& typeName);

    /**
     * @brief Register a method for a type
     * @param typeName Type name
     * @param methodName Method name
     * @param signature Method signature
     * @param invoker Method invoker function
     */
    static void RegisterMethod(const std::string& typeName, const std::string& methodName, const std::string& signature,
                               std::function<void*()> invoker);

    /**
     * @brief Get methods for a registered type
     * @param typeName Type name
     * @return Vector of method information
     */
    static std::vector<MethodInfo> GetMethods(const std::string& typeName);

    /**
     * @brief Check if type has a specific method
     * @param typeName Type name
     * @param methodName Method name
     * @return True if method exists
     */
    static bool HasMethod(const std::string& typeName, const std::string& methodName);

    /**
     * @brief Invoke a method by name
     * @param typeName Type name
     * @param methodName Method name
     * @param instance Instance to call method on (optional)
     * @return Method result as void pointer
     */
    static void* InvokeMethod(const std::string& typeName, const std::string& methodName,
                              std::shared_ptr<void> instance = nullptr);

    /**
     * @brief Get size of registered type
     * @param typeName Type name
     * @return Size in bytes, or 0 if not found
     */
    static size_t GetTypeSize(const std::string& typeName);

    /**
     * @brief Check if two types are the same
     * @param typeName1 First type name
     * @param typeName2 Second type name
     * @return True if types are the same
     */
    static bool AreTypesSame(const std::string& typeName1, const std::string& typeName2);

    /**
     * @brief Get base class names for a type (if available)
     * @param typeName Type name
     * @return Vector of base class names
     */
    static std::vector<std::string> GetBaseClasses(const std::string& typeName);

    /**
     * @brief Check if type derives from another type
     * @param derivedType Derived type name
     * @param baseType Base type name
     * @return True if derivedType derives from baseType
     */
    static bool IsTypeDeriviedFrom(const std::string& derivedType, const std::string& baseType);

    /**
     * @brief Clear all registered types and methods
     */
    static void ClearRegistry();

    /**
     * @brief Get number of registered types
     * @return Number of registered types
     */
    static size_t GetRegisteredTypeCount();

  private:
    /**
     * @brief Get the type registry (singleton pattern)
     * @return Reference to type registry map
     */
    static std::unordered_map<std::string, FactoryFunction>& GetTypeRegistry();

    /**
     * @brief Get the type info registry (singleton pattern)
     * @return Reference to type info registry map
     */
    static std::unordered_map<std::string, TypeInfo>& GetTypeInfoRegistry();

    /**
     * @brief Get the method registry (singleton pattern)
     * @return Reference to method registry map
     */
    static std::unordered_map<std::string, std::vector<MethodInfo>>& GetMethodRegistry();

    /**
     * @brief Demangle a C++ type name to human-readable form
     * @param mangledName Mangled type name from typeid
     * @return Demangled type name
     */
    static std::string DemangleTypeName(const std::string& mangledName);

    /**
     * @brief Extract clean class name from full qualified name
     * @param fullName Full qualified name
     * @return Clean class name without namespace
     */
    static std::string ExtractClassName(const std::string& fullName);
};

/**
 * @brief Automatic type registration helper
 * @tparam T Type to register
 */
template <typename T>
class TypeRegistrar
{
  public:
    /**
     * @brief Constructor automatically registers the type
     * @param name Optional type name
     */
    explicit TypeRegistrar(const std::string& name = "")
    {
        AssemblyExtensions::RegisterType<T>(name);
    }
};

/**
 * @brief Macro to easily register a type
 */
#define REGISTER_TYPE(Type) static neo::extensions::TypeRegistrar<Type> g_##Type##_registrar(#Type)

/**
 * @brief Macro to register a type with custom name
 */
#define REGISTER_TYPE_AS(Type, Name) static neo::extensions::TypeRegistrar<Type> g_##Type##_registrar(Name)
}  // namespace neo::extensions
