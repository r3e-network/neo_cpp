#include <neo/extensions/assembly_extensions.h>
#include <mutex>
#include <typeinfo>
#include <cstring>

#ifdef __GNUC__
#include <cxxabi.h>
#endif

namespace neo::extensions
{
    // Thread-safe singleton registries
    std::unordered_map<std::string, AssemblyExtensions::FactoryFunction>& AssemblyExtensions::GetTypeRegistry()
    {
        static std::unordered_map<std::string, FactoryFunction> registry;
        return registry;
    }

    std::unordered_map<std::string, AssemblyExtensions::TypeInfo>& AssemblyExtensions::GetTypeInfoRegistry()
    {
        static std::unordered_map<std::string, TypeInfo> registry;
        return registry;
    }

    std::unordered_map<std::string, std::vector<AssemblyExtensions::MethodInfo>>& AssemblyExtensions::GetMethodRegistry()
    {
        static std::unordered_map<std::string, std::vector<MethodInfo>> registry;
        return registry;
    }

    bool AssemblyExtensions::HasType(const std::string& typeName)
    {
        static std::mutex registryMutex;
        std::lock_guard<std::mutex> lock(registryMutex);
        
        const auto& registry = GetTypeRegistry();
        return registry.find(typeName) != registry.end();
    }

    std::shared_ptr<void> AssemblyExtensions::CreateInstance(const std::string& typeName)
    {
        static std::mutex registryMutex;
        std::lock_guard<std::mutex> lock(registryMutex);
        
        const auto& registry = GetTypeRegistry();
        auto it = registry.find(typeName);
        if (it != registry.end())
        {
            return it->second();
        }
        return nullptr;
    }

    std::vector<std::string> AssemblyExtensions::GetRegisteredTypes()
    {
        static std::mutex registryMutex;
        std::lock_guard<std::mutex> lock(registryMutex);
        
        std::vector<std::string> types;
        const auto& registry = GetTypeRegistry();
        
        types.reserve(registry.size());
        for (const auto& pair : registry)
        {
            types.push_back(pair.first);
        }
        
        return types;
    }

    AssemblyExtensions::TypeInfo AssemblyExtensions::GetRegisteredTypeInfo(const std::string& typeName)
    {
        static std::mutex registryMutex;
        std::lock_guard<std::mutex> lock(registryMutex);
        
        const auto& registry = GetTypeInfoRegistry();
        auto it = registry.find(typeName);
        if (it != registry.end())
        {
            return it->second;
        }
        return TypeInfo{};
    }

    void AssemblyExtensions::RegisterMethod(const std::string& typeName, 
                                          const std::string& methodName,
                                          const std::string& signature,
                                          std::function<void*()> invoker)
    {
        static std::mutex registryMutex;
        std::lock_guard<std::mutex> lock(registryMutex);
        
        auto& methodRegistry = GetMethodRegistry();
        MethodInfo method(methodName, signature);
        method.invoker = invoker;
        methodRegistry[typeName].push_back(method);
    }

    std::vector<AssemblyExtensions::MethodInfo> AssemblyExtensions::GetMethods(const std::string& typeName)
    {
        static std::mutex registryMutex;
        std::lock_guard<std::mutex> lock(registryMutex);
        
        const auto& registry = GetMethodRegistry();
        auto it = registry.find(typeName);
        if (it != registry.end())
        {
            return it->second;
        }
        return {};
    }

    bool AssemblyExtensions::HasMethod(const std::string& typeName, const std::string& methodName)
    {
        static std::mutex registryMutex;
        std::lock_guard<std::mutex> lock(registryMutex);
        
        const auto& registry = GetMethodRegistry();
        auto it = registry.find(typeName);
        if (it != registry.end())
        {
            for (const auto& method : it->second)
            {
                if (method.name == methodName)
                    return true;
            }
        }
        return false;
    }

    void* AssemblyExtensions::InvokeMethod(const std::string& typeName, 
                                         const std::string& methodName,
                                         std::shared_ptr<void> instance)
    {
        static std::mutex registryMutex;
        std::lock_guard<std::mutex> lock(registryMutex);
        
        const auto& registry = GetMethodRegistry();
        auto it = registry.find(typeName);
        if (it != registry.end())
        {
            for (const auto& method : it->second)
            {
                if (method.name == methodName && method.invoker)
                {
                    return method.invoker();
                }
            }
        }
        return nullptr;
    }

    size_t AssemblyExtensions::GetTypeSize(const std::string& typeName)
    {
        auto info = GetRegisteredTypeInfo(typeName);
        return info.size;
    }

    bool AssemblyExtensions::AreTypesSame(const std::string& typeName1, const std::string& typeName2)
    {
        return typeName1 == typeName2;
    }

    std::vector<std::string> AssemblyExtensions::GetBaseClasses(const std::string& typeName)
    {
        // Basic implementation - would need enhanced RTTI for full inheritance info
        (void)typeName; // Suppress unused parameter warning
        return {};
    }

    bool AssemblyExtensions::IsTypeDeriviedFrom(const std::string& derivedType, const std::string& baseType)
    {
        // Basic implementation - would need enhanced RTTI for inheritance checking
        (void)derivedType; // Suppress unused parameter warning
        (void)baseType; // Suppress unused parameter warning
        return false;
    }

    void AssemblyExtensions::ClearRegistry()
    {
        static std::mutex registryMutex;
        std::lock_guard<std::mutex> lock(registryMutex);
        
        GetTypeRegistry().clear();
        GetTypeInfoRegistry().clear();
        GetMethodRegistry().clear();
    }

    size_t AssemblyExtensions::GetRegisteredTypeCount()
    {
        static std::mutex registryMutex;
        std::lock_guard<std::mutex> lock(registryMutex);
        
        return GetTypeRegistry().size();
    }

    std::string AssemblyExtensions::DemangleTypeName(const std::string& mangledName)
    {
#ifdef __GNUC__
        int status = 0;
        char* demangled = abi::__cxa_demangle(mangledName.c_str(), nullptr, nullptr, &status);
        
        if (status == 0 && demangled)
        {
            std::string result(demangled);
            free(demangled);
            return ExtractClassName(result);
        }
#endif
        
        // Fallback for other compilers or if demangling fails
        return ExtractClassName(mangledName);
    }

    std::string AssemblyExtensions::ExtractClassName(const std::string& fullName)
    {
        // Extract class name from fully qualified name
        size_t lastNamespace = fullName.find_last_of("::");
        if (lastNamespace != std::string::npos && lastNamespace > 0)
        {
            return fullName.substr(lastNamespace + 1);
        }
        
        // Remove common template/pointer markers
        std::string cleaned = fullName;
        
        // Remove template parameters
        size_t templateStart = cleaned.find('<');
        if (templateStart != std::string::npos)
        {
            cleaned = cleaned.substr(0, templateStart);
        }
        
        // Remove pointer/reference markers
        size_t pointerPos = cleaned.find('*');
        if (pointerPos != std::string::npos)
        {
            cleaned = cleaned.substr(0, pointerPos);
        }
        
        size_t refPos = cleaned.find('&');
        if (refPos != std::string::npos)
        {
            cleaned = cleaned.substr(0, refPos);
        }
        
        // Trim whitespace
        size_t start = cleaned.find_first_not_of(" \t");
        size_t end = cleaned.find_last_not_of(" \t");
        
        if (start != std::string::npos && end != std::string::npos)
        {
            return cleaned.substr(start, end - start + 1);
        }
        
        return cleaned;
    }
}
