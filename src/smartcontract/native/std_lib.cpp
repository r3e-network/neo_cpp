/**
 * @file std_lib.cpp
 * @brief Std Lib
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <algorithm>
// #include <codecvt> // Removed - using manual UTF-8 parsing instead
#include <neo/cryptography/base58.h>
#include <neo/cryptography/base64.h>
#include <neo/cryptography/base64url.h>
#include <neo/cryptography/hash.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/binary_serializer.h>
#include <neo/smartcontract/json_serializer.h>
#include <neo/smartcontract/native/std_lib.h>

#include <cwctype>
#include <iomanip>
#include <locale>
#include <sstream>

namespace neo::smartcontract::native
{
StdLib::StdLib() : NativeContract(NAME, ID) {}

void StdLib::Initialize()
{
    RegisterMethod("serialize", CallFlags::None,
                   std::bind(&StdLib::OnSerialize, this, std::placeholders::_1, std::placeholders::_2));
    RegisterMethod("deserialize", CallFlags::None,
                   std::bind(&StdLib::OnDeserialize, this, std::placeholders::_1, std::placeholders::_2));
    RegisterMethod("jsonSerialize", CallFlags::None,
                   std::bind(&StdLib::OnJsonSerialize, this, std::placeholders::_1, std::placeholders::_2));
    RegisterMethod("jsonDeserialize", CallFlags::None,
                   std::bind(&StdLib::OnJsonDeserialize, this, std::placeholders::_1, std::placeholders::_2));
    RegisterMethod("itoa", CallFlags::None,
                   std::bind(&StdLib::OnItoa, this, std::placeholders::_1, std::placeholders::_2));
    RegisterMethod("atoi", CallFlags::None,
                   std::bind(&StdLib::OnAtoi, this, std::placeholders::_1, std::placeholders::_2));
    RegisterMethod("base64Encode", CallFlags::None,
                   std::bind(&StdLib::OnBase64Encode, this, std::placeholders::_1, std::placeholders::_2));
    RegisterMethod("base64Decode", CallFlags::None,
                   std::bind(&StdLib::OnBase64Decode, this, std::placeholders::_1, std::placeholders::_2));
    RegisterMethod("base64UrlEncode", CallFlags::None,
                   std::bind(&StdLib::OnBase64UrlEncode, this, std::placeholders::_1, std::placeholders::_2));
    RegisterMethod("base64UrlDecode", CallFlags::None,
                   std::bind(&StdLib::OnBase64UrlDecode, this, std::placeholders::_1, std::placeholders::_2));
    RegisterMethod("base58Encode", CallFlags::None,
                   std::bind(&StdLib::OnBase58Encode, this, std::placeholders::_1, std::placeholders::_2));
    RegisterMethod("base58Decode", CallFlags::None,
                   std::bind(&StdLib::OnBase58Decode, this, std::placeholders::_1, std::placeholders::_2));
    RegisterMethod("base58CheckEncode", CallFlags::None,
                   std::bind(&StdLib::OnBase58CheckEncode, this, std::placeholders::_1, std::placeholders::_2));
    RegisterMethod("base58CheckDecode", CallFlags::None,
                   std::bind(&StdLib::OnBase58CheckDecode, this, std::placeholders::_1, std::placeholders::_2));
    RegisterMethod("memoryCompare", CallFlags::None,
                   std::bind(&StdLib::OnMemoryCompare, this, std::placeholders::_1, std::placeholders::_2));
    RegisterMethod("memoryCopy", CallFlags::None,
                   std::bind(&StdLib::OnMemoryCopy, this, std::placeholders::_1, std::placeholders::_2));
    RegisterMethod("memorySearch", CallFlags::None,
                   std::bind(&StdLib::OnMemorySearch, this, std::placeholders::_1, std::placeholders::_2));
    RegisterMethod("stringCompare", CallFlags::None,
                   std::bind(&StdLib::OnStringCompare, this, std::placeholders::_1, std::placeholders::_2));
    RegisterMethod("strLen", CallFlags::None,
                   std::bind(&StdLib::OnStrLen, this, std::placeholders::_1, std::placeholders::_2));
}

std::shared_ptr<vm::StackItem> StdLib::OnSerialize(ApplicationEngine& engine,
                                                   const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    if (args.empty()) throw std::runtime_error("Invalid arguments");

    auto item = args[0];

    // Use BinarySerializer exactly like C# BinarySerializer.Serialize(item, engine.Limits)
    try
    {
        auto serialized =
            BinarySerializer::Serialize(item, engine.GetLimits().MaxItemSize, engine.GetLimits().MaxStackSize);
        return vm::StackItem::Create(serialized);
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error("Serialization failed: " + std::string(e.what()));
    }
}

std::shared_ptr<vm::StackItem> StdLib::OnDeserialize(ApplicationEngine& engine,
                                                     const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    if (args.empty()) throw std::runtime_error("Invalid arguments");

    auto dataItem = args[0];
    auto data = dataItem->GetByteArray();

    // Use BinarySerializer exactly like C# BinarySerializer.Deserialize(data, engine.Limits, engine.ReferenceCounter)
    try
    {
        return BinarySerializer::Deserialize(data.AsSpan(), engine.GetLimits().MaxItemSize,
                                             engine.GetLimits().MaxStackSize);
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error("Deserialization failed: " + std::string(e.what()));
    }
}

std::shared_ptr<vm::StackItem> StdLib::OnJsonSerialize(ApplicationEngine& engine,
                                                       const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    if (args.empty()) throw std::runtime_error("Invalid arguments");

    auto item = args[0];

    // Use JsonSerializer exactly like C# JsonSerializer.SerializeToByteArray(item, engine.Limits.MaxItemSize)
    try
    {
        auto serialized = JsonSerializer::SerializeToByteArray(item, engine.GetLimits().MaxItemSize);
        return vm::StackItem::Create(serialized);
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error("JSON serialization failed: " + std::string(e.what()));
    }
}

std::shared_ptr<vm::StackItem> StdLib::OnJsonDeserialize(ApplicationEngine& engine,
                                                         const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    if (args.empty()) throw std::runtime_error("Invalid arguments");

    auto dataItem = args[0];
    auto data = dataItem->GetByteArray();

    // Use JsonSerializer exactly like C# JsonSerializer.Deserialize(engine, JToken.Parse(json, 10), engine.Limits,
    // engine.ReferenceCounter)
    try
    {
        return JsonSerializer::Deserialize(data.AsSpan(), engine.GetLimits().MaxItemSize,
                                           engine.GetLimits().MaxStackSize);
    }
    catch (const std::exception&)
    {
        return vm::StackItem::Create(data);  // Return original string if parsing fails
    }
}

std::shared_ptr<vm::StackItem> StdLib::OnItoa(ApplicationEngine& engine,
                                              const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    (void)engine;  // Suppress unused parameter warning

    if (args.empty()) throw std::runtime_error("Invalid arguments");

    auto valueItem = args[0];
    auto value = valueItem->GetInteger();

    // Get the base if provided
    int base = 10;
    if (args.size() > 1)
    {
        auto baseItem = args[1];
        base = static_cast<int>(baseItem->GetInteger());

        // Check if the base is valid
        if (base != 10 && base != 16) throw std::runtime_error("Invalid base");
    }

    // Convert to string
    std::string result;
    if (base == 10)
    {
        result = std::to_string(value);
    }
    else if (base == 16)
    {
        std::ostringstream stream;
        stream << std::hex << value;
        result = stream.str();
    }

    return vm::StackItem::Create(result);
}

std::shared_ptr<vm::StackItem> StdLib::OnAtoi(ApplicationEngine& engine,
                                              const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    (void)engine;  // Suppress unused parameter warning

    if (args.empty()) throw std::runtime_error("Invalid arguments");

    auto valueItem = args[0];
    auto value = valueItem->GetString();

    // Get the base if provided
    int base = 10;
    if (args.size() > 1)
    {
        auto baseItem = args[1];
        base = static_cast<int>(baseItem->GetInteger());

        // Check if the base is valid
        if (base != 10 && base != 16) throw std::runtime_error("Invalid base");
    }

    // Convert to integer
    int64_t result = 0;
    try
    {
        if (base == 10)
        {
            result = std::stoll(value);
        }
        else if (base == 16)
        {
            result = std::stoll(value, nullptr, 16);
        }
    }
    catch (const std::exception&)
    {
        throw std::runtime_error("Invalid number format");
    }

    return vm::StackItem::Create(result);
}

std::shared_ptr<vm::StackItem> StdLib::OnBase64Encode(ApplicationEngine& engine,
                                                      const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    (void)engine;  // Suppress unused parameter warning

    if (args.empty()) throw std::runtime_error("Invalid arguments");

    auto dataItem = args[0];
    auto data = dataItem->GetByteArray();

    // Encode to Base64
    std::string result = cryptography::Base64::Encode(data.AsSpan());

    return vm::StackItem::Create(result);
}

std::shared_ptr<vm::StackItem> StdLib::OnBase64Decode(ApplicationEngine& engine,
                                                      const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    (void)engine;  // Suppress unused parameter warning

    if (args.empty()) throw std::runtime_error("Invalid arguments");

    auto dataItem = args[0];
    auto data = dataItem->GetString();

    // Decode from Base64
    io::ByteVector result = cryptography::Base64::Decode(data);

    return vm::StackItem::Create(result);
}

std::shared_ptr<vm::StackItem> StdLib::OnBase58Encode(ApplicationEngine& engine,
                                                      const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    (void)engine;  // Suppress unused parameter warning

    if (args.empty()) throw std::runtime_error("Invalid arguments");

    auto dataItem = args[0];
    auto data = dataItem->GetByteArray();

    // Encode to Base58
    std::string result = neo::cryptography::Base58::Encode(data);

    return vm::StackItem::Create(result);
}

std::shared_ptr<vm::StackItem> StdLib::OnBase58Decode(ApplicationEngine& engine,
                                                      const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    (void)engine;  // Suppress unused parameter warning

    if (args.empty()) throw std::runtime_error("Invalid arguments");

    auto dataItem = args[0];
    auto data = dataItem->GetString();

    // Decode from Base58
    io::ByteVector result = neo::cryptography::Base58::DecodeToByteVector(data);

    return vm::StackItem::Create(result);
}

std::shared_ptr<vm::StackItem> StdLib::OnBase58CheckEncode(ApplicationEngine& engine,
                                                           const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    (void)engine;  // Suppress unused parameter warning

    if (args.empty()) throw std::runtime_error("Invalid arguments");

    auto dataItem = args[0];
    auto data = dataItem->GetByteArray();

    // Encode to Base58Check
    std::string result = neo::cryptography::Base58::EncodeCheck(data);

    return vm::StackItem::Create(result);
}

std::shared_ptr<vm::StackItem> StdLib::OnBase58CheckDecode(ApplicationEngine& engine,
                                                           const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    (void)engine;  // Suppress unused parameter warning

    if (args.empty()) throw std::runtime_error("Invalid arguments");

    auto dataItem = args[0];
    auto data = dataItem->GetString();

    // Decode from Base58Check
    io::ByteVector result = neo::cryptography::Base58::DecodeCheckToByteVector(data);

    return vm::StackItem::Create(result);
}

std::shared_ptr<vm::StackItem> StdLib::OnMemoryCompare(ApplicationEngine& engine,
                                                       const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    (void)engine;  // Suppress unused parameter warning

    if (args.size() < 2) throw std::runtime_error("Invalid arguments");

    auto data1Item = args[0];
    auto data2Item = args[1];

    auto data1 = data1Item->GetByteArray();
    auto data2 = data2Item->GetByteArray();

    // Compare memory
    int result = std::memcmp(data1.Data(), data2.Data(), std::min(data1.Size(), data2.Size()));
    if (result == 0)
    {
        if (data1.Size() < data2.Size())
            result = -1;
        else if (data1.Size() > data2.Size())
            result = 1;
    }

    return vm::StackItem::Create(static_cast<int64_t>(result));
}

std::shared_ptr<vm::StackItem> StdLib::OnMemoryCopy(ApplicationEngine& engine,
                                                    const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    (void)engine;  // Suppress unused parameter warning

    if (args.size() < 3) throw std::runtime_error("Invalid arguments");

    auto sourceItem = args[0];
    auto destItem = args[1];
    auto lengthItem = args[2];

    auto source = sourceItem->GetByteArray();
    auto dest = destItem->GetByteArray();
    auto length = lengthItem->GetInteger();

    // Copy memory
    if (length <= 0 || length > static_cast<int64_t>(source.Size()) || length > static_cast<int64_t>(dest.Size()))
        throw std::runtime_error("Invalid length");

    std::memcpy(const_cast<uint8_t*>(dest.Data()), source.Data(), static_cast<size_t>(length));

    return vm::StackItem::Create(true);
}

std::shared_ptr<vm::StackItem> StdLib::OnMemorySearch(ApplicationEngine& engine,
                                                      const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    (void)engine;  // Suppress unused parameter warning

    if (args.size() < 2) throw std::runtime_error("Invalid arguments");

    auto sourceItem = args[0];
    auto valueItem = args[1];

    auto source = sourceItem->GetByteArray();
    auto value = valueItem->GetByteArray();

    // Search memory
    if (value.Size() > source.Size()) return vm::StackItem::Create(static_cast<int64_t>(-1));

    for (size_t i = 0; i <= source.Size() - value.Size(); i++)
    {
        if (std::memcmp(source.Data() + i, value.Data(), value.Size()) == 0)
            return vm::StackItem::Create(static_cast<int64_t>(i));
    }

    return vm::StackItem::Create(static_cast<int64_t>(-1));
}

std::shared_ptr<vm::StackItem> StdLib::OnStringCompare(ApplicationEngine& engine,
                                                       const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    (void)engine;  // Suppress unused parameter warning

    if (args.size() < 2) throw std::runtime_error("Invalid arguments");

    auto string1Item = args[0];
    auto string2Item = args[1];

    auto string1 = string1Item->GetString();
    auto string2 = string2Item->GetString();

    // Compare strings
    int result = string1.compare(string2);

    return vm::StackItem::Create(static_cast<int64_t>(result));
}

std::shared_ptr<vm::StackItem> StdLib::OnBase64UrlEncode(ApplicationEngine& engine,
                                                         const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    (void)engine;  // Suppress unused parameter warning

    if (args.empty()) throw std::runtime_error("Invalid arguments");

    auto dataItem = args[0];

    // Check if the data is a string or a byte array
    std::string result;
    try
    {
        // Try to get as string first
        auto data = dataItem->GetString();
        result = cryptography::Base64Url::Encode(data);
    }
    catch (const std::exception&)
    {
        // If that fails, try as byte array
        auto data = dataItem->GetByteArray();
        result = cryptography::Base64Url::Encode(data.AsSpan());
    }

    return vm::StackItem::Create(result);
}

std::shared_ptr<vm::StackItem> StdLib::OnBase64UrlDecode(ApplicationEngine& engine,
                                                         const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    (void)engine;  // Suppress unused parameter warning

    if (args.empty()) throw std::runtime_error("Invalid arguments");

    auto dataItem = args[0];
    auto data = dataItem->GetString();

    // Decode from Base64Url
    std::string result = cryptography::Base64Url::DecodeToString(data);

    return vm::StackItem::Create(result);
}

std::shared_ptr<vm::StackItem> StdLib::OnStrLen(ApplicationEngine& engine,
                                                const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    (void)engine;  // Suppress unused parameter warning

    if (args.empty()) throw std::runtime_error("Invalid arguments");

    auto stringItem = args[0];
    auto str = stringItem->GetString();

    // Count UTF-8 characters properly without deprecated codecvt
    int count = 0;
    for (size_t i = 0; i < str.length();)
    {
        unsigned char c = str[i];
        if (c < 0x80)
        {
            // Single byte character
            i++;
        }
        else if ((c & 0xe0) == 0xc0)
        {
            // Two byte character
            i += 2;
        }
        else if ((c & 0xf0) == 0xe0)
        {
            // Three byte character
            i += 3;
        }
        else if ((c & 0xf8) == 0xf0)
        {
            // Four byte character
            i += 4;
        }
        else
        {
            // Invalid UTF-8, count as single byte
            i++;
        }
        count++;
    }

    return vm::StackItem::Create(static_cast<int64_t>(count));
}
}  // namespace neo::smartcontract::native
