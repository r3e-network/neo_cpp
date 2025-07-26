#include <neo/cli/type_converters.h>
#include <neo/io/uint256.h>
#include <neo/io/uint160.h>
#include <neo/core/safe_conversions.h>

// IMPORTANT: This interface returns raw pointers for backward compatibility.
// Callers MUST take ownership and delete the returned pointers.
// Consider using SafeTypeConverters for new code which provides RAII safety.
#include <neo/cryptography/ecc/ecpoint.h>
#include <algorithm>
#include <cctype>

namespace neo::cli
{
    TypeConverters& TypeConverters::Instance()
    {
        static TypeConverters instance;
        return instance;
    }

    TypeConverters::TypeConverters()
    {
        InitializeDefaultConverters();
    }

    void TypeConverters::RegisterConverter(const std::string& typeName, const TypeConverter& converter)
    {
        converters_[typeName] = converter;
    }

    TypeConverter TypeConverters::GetConverter(const std::string& typeName) const
    {
        auto it = converters_.find(typeName);
        if (it != converters_.end())
            return it->second;

        return nullptr;
    }

    bool TypeConverters::HasConverter(const std::string& typeName) const
    {
        return converters_.find(typeName) != converters_.end();
    }

    const std::unordered_map<std::string, TypeConverter>& TypeConverters::GetAllConverters() const
    {
        return converters_;
    }

    void TypeConverters::InitializeDefaultConverters()
    {
        // Register string converter - WARNING: Returns raw pointer, caller must delete
        RegisterConverter("string", [](const std::vector<std::string>& args, bool canConsumeAll) -> void* {
            try {
                if (args.empty())
                    return new std::string("");

                if (canConsumeAll)
                {
                    std::string result;
                    for (size_t i = 0; i < args.size(); i++)
                    {
                        if (i > 0)
                            result += " ";
                        result += args[i];
                    }
                    return new std::string(result);
                }

                return new std::string(args[0]);
            } catch (const std::bad_alloc& e) {
                throw std::runtime_error("Memory allocation failed for string conversion: " + std::string(e.what()));
            }
        });

        // Register UInt256 converter - WARNING: Returns raw pointer, caller must delete
        RegisterConverter("UInt256", [](const std::vector<std::string>& args, bool canConsumeAll) -> void* {
            (void)canConsumeAll; // Suppress unused parameter warning
            if (args.empty())
                throw std::runtime_error("Missing argument for UInt256");

            try {
                return new io::UInt256(io::UInt256::Parse(args[0]));
            } catch (const std::bad_alloc& e) {
                throw std::runtime_error("Memory allocation failed for UInt256 conversion: " + std::string(e.what()));
            } catch (const std::exception& e) {
                throw std::runtime_error("Invalid UInt256 format: " + std::string(e.what()));
            }
        });

        // Register UInt160 converter - WARNING: Returns raw pointer, caller must delete
        RegisterConverter("UInt160", [](const std::vector<std::string>& args, bool canConsumeAll) -> void* {
            (void)canConsumeAll; // Suppress unused parameter warning
            if (args.empty())
                throw std::runtime_error("Missing argument for UInt160");

            try {
                return new io::UInt160(io::UInt160::Parse(args[0]));
            } catch (const std::bad_alloc& e) {
                throw std::runtime_error("Memory allocation failed for UInt160 conversion: " + std::string(e.what()));
            } catch (const std::exception& e) {
                throw std::runtime_error("Invalid UInt160 format: " + std::string(e.what()));
            }
        });

        // Register ECPoint converter
        RegisterConverter("ECPoint", [](const std::vector<std::string>& args, bool canConsumeAll) -> void* {
            (void)canConsumeAll; // Suppress unused parameter warning
            if (args.empty())
                throw std::runtime_error("Missing argument for ECPoint");

            return new neo::cryptography::ecc::ECPoint(neo::cryptography::ecc::ECPoint::Parse(args[0]));
        });

        // Register int converter
        RegisterConverter("int", [](const std::vector<std::string>& args, bool canConsumeAll) -> void* {
            (void)canConsumeAll; // Suppress unused parameter warning
            if (args.empty())
                throw std::runtime_error("Missing argument for int");

            return new int(std::stoi(args[0]));
        });

        // Register uint32_t converter
        RegisterConverter("uint32_t", [](const std::vector<std::string>& args, bool canConsumeAll) -> void* {
            (void)canConsumeAll; // Suppress unused parameter warning
            if (args.empty())
                throw std::runtime_error("Missing argument for uint32_t");

            return new uint32_t(std::stoul(args[0]));
        });

        // Register bool converter
        RegisterConverter("bool", [](const std::vector<std::string>& args, bool canConsumeAll) -> void* {
            (void)canConsumeAll; // Suppress unused parameter warning
            if (args.empty())
                throw std::runtime_error("Missing argument for bool");

            std::string value = args[0];
            std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return std::tolower(c); });

            bool result = (value == "true" || value == "yes" || value == "y" || value == "1");
            return new bool(result);
        });
    }
}
