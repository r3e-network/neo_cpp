#include <neo/cli/type_converters.h>
#include <neo/io/uint256.h>
#include <neo/io/uint160.h>
#include <neo/cryptography/ecc/ec_point.h>
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
        // Register string converter
        RegisterConverter("string", [](const std::vector<std::string>& args, bool canConsumeAll) -> void* {
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
        });

        // Register UInt256 converter
        RegisterConverter("UInt256", [](const std::vector<std::string>& args, bool canConsumeAll) -> void* {
            if (args.empty())
                throw std::runtime_error("Missing argument for UInt256");

            return new io::UInt256(io::UInt256::Parse(args[0]));
        });

        // Register UInt160 converter
        RegisterConverter("UInt160", [](const std::vector<std::string>& args, bool canConsumeAll) -> void* {
            if (args.empty())
                throw std::runtime_error("Missing argument for UInt160");

            return new io::UInt160(io::UInt160::Parse(args[0]));
        });

        // Register ECPoint converter
        RegisterConverter("ECPoint", [](const std::vector<std::string>& args, bool canConsumeAll) -> void* {
            if (args.empty())
                throw std::runtime_error("Missing argument for ECPoint");

            return new cryptography::ecc::ECPoint(cryptography::ecc::ECPoint::Parse(args[0]));
        });

        // Register int converter
        RegisterConverter("int", [](const std::vector<std::string>& args, bool canConsumeAll) -> void* {
            if (args.empty())
                throw std::runtime_error("Missing argument for int");

            return new int(std::stoi(args[0]));
        });

        // Register uint32_t converter
        RegisterConverter("uint32_t", [](const std::vector<std::string>& args, bool canConsumeAll) -> void* {
            if (args.empty())
                throw std::runtime_error("Missing argument for uint32_t");

            return new uint32_t(std::stoul(args[0]));
        });

        // Register bool converter
        RegisterConverter("bool", [](const std::vector<std::string>& args, bool canConsumeAll) -> void* {
            if (args.empty())
                throw std::runtime_error("Missing argument for bool");

            std::string value = args[0];
            std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return std::tolower(c); });

            bool result = (value == "true" || value == "yes" || value == "y" || value == "1");
            return new bool(result);
        });
    }
}
