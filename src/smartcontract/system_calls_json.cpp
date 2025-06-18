#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/system_calls.h>
#include <neo/vm/compound_items.h>

namespace neo::smartcontract
{
    // This file contains the implementation of JSON-related system calls

    namespace
    {
        void RegisterJsonSystemCallsImpl(ApplicationEngine& engine)
        {
            // System.Json.Serialize
            engine.RegisterSystemCall("System.Json.Serialize", [](vm::ExecutionEngine& engine) {
                auto& appEngine = static_cast<ApplicationEngine&>(engine);
                auto& context = appEngine.GetCurrentContext();

                auto item = context.Pop();

                // Helper function to convert a stack item to JSON
                std::function<std::string(const std::shared_ptr<vm::StackItem>&)> stackItemToJson;
                stackItemToJson = [&stackItemToJson](const std::shared_ptr<vm::StackItem>& item) -> std::string {
                    if (item->IsNull())
                    {
                        return "null";
                    }
                    else if (item->IsBoolean())
                    {
                        return item->GetBoolean() ? "true" : "false";
                    }
                    else if (item->IsInteger())
                    {
                        return std::to_string(item->GetInteger());
                    }
                    else if (item->IsByteString())
                    {
                        auto bytes = item->GetByteArray();
                        std::string result = "\"";
                        for (size_t i = 0; i < bytes.Size(); i++)
                        {
                            char buf[3];
                            snprintf(buf, sizeof(buf), "%02x", bytes[i]);
                            result += buf;
                        }
                        result += "\"";
                        return result;
                    }
                    else if (item->IsArray())
                    {
                        auto array = item->GetArray();
                        std::string result = "[";
                        for (size_t i = 0; i < array.size(); i++)
                        {
                            if (i > 0)
                                result += ",";
                            result += stackItemToJson(array[i]);
                        }
                        result += "]";
                        return result;
                    }
                    else if (item->IsMap())
                    {
                        auto map = item->GetMap();
                        std::string result = "{";
                        bool first = true;
                        for (const auto& pair : map)
                        {
                            if (!first)
                                result += ",";
                            first = false;
                            result += "\"" + pair.first->GetString() + "\":" + stackItemToJson(pair.second);
                        }
                        result += "}";
                        return result;
                    }
                    else if (item->IsInteropInterface())
                    {
                        return "\"<interop interface>\"";
                    }
                    else
                    {
                        return "\"<unknown>\"";
                    }
                };

                // Convert the stack item to JSON
                std::string json = stackItemToJson(item);
                context.Push(vm::StackItem::Create(json));
                return true;
            }, 1 << 15);

            // System.Json.Deserialize
            engine.RegisterSystemCall("System.Json.Deserialize", [](vm::ExecutionEngine& engine) {
                auto& appEngine = static_cast<ApplicationEngine&>(engine);
                auto& context = appEngine.GetCurrentContext();

                auto jsonItem = context.Pop();
                auto json = jsonItem->GetString();

                // Helper function to parse JSON
                std::function<std::shared_ptr<vm::StackItem>(const std::string&, size_t&)> parseJson;
                parseJson = [&parseJson](const std::string& json, size_t& pos) -> std::shared_ptr<vm::StackItem> {
                    // Skip whitespace
                    while (pos < json.size() && std::isspace(json[pos]))
                        pos++;

                    if (pos >= json.size())
                        throw std::runtime_error("Unexpected end of JSON");

                    // Parse based on the first character
                    char c = json[pos];
                    if (c == 'n')
                    {
                        // Parse null
                        if (pos + 4 <= json.size() && json.substr(pos, 4) == "null")
                        {
                            pos += 4;
                            return vm::StackItem::Create(nullptr);
                        }
                        throw std::runtime_error("Invalid JSON: expected 'null'");
                    }
                    else if (c == 't')
                    {
                        // Parse true
                        if (pos + 4 <= json.size() && json.substr(pos, 4) == "true")
                        {
                            pos += 4;
                            return vm::StackItem::Create(true);
                        }
                        throw std::runtime_error("Invalid JSON: expected 'true'");
                    }
                    else if (c == 'f')
                    {
                        // Parse false
                        if (pos + 5 <= json.size() && json.substr(pos, 5) == "false")
                        {
                            pos += 5;
                            return vm::StackItem::Create(false);
                        }
                        throw std::runtime_error("Invalid JSON: expected 'false'");
                    }
                    else if (c == '"')
                    {
                        // Parse string
                        pos++; // Skip opening quote
                        std::string str;
                        while (pos < json.size() && json[pos] != '"')
                        {
                            if (json[pos] == '\\')
                            {
                                pos++;
                                if (pos >= json.size())
                                    throw std::runtime_error("Unexpected end of JSON");

                                switch (json[pos])
                                {
                                    case '"': str += '"'; break;
                                    case '\\': str += '\\'; break;
                                    case '/': str += '/'; break;
                                    case 'b': str += '\b'; break;
                                    case 'f': str += '\f'; break;
                                    case 'n': str += '\n'; break;
                                    case 'r': str += '\r'; break;
                                    case 't': str += '\t'; break;
                                    default: str += json[pos]; break;
                                }
                            }
                            else
                            {
                                str += json[pos];
                            }
                            pos++;
                        }

                        if (pos >= json.size())
                            throw std::runtime_error("Unexpected end of JSON");

                        pos++; // Skip closing quote
                        return vm::StackItem::Create(str);
                    }
                    else if (c == '[')
                    {
                        // Parse array
                        pos++; // Skip opening bracket
                        auto array = vm::StackItem::CreateArray();

                        // Skip whitespace
                        while (pos < json.size() && std::isspace(json[pos]))
                            pos++;

                        if (pos < json.size() && json[pos] == ']')
                        {
                            pos++; // Skip closing bracket
                            return array;
                        }

                        while (true)
                        {
                            auto item = parseJson(json, pos);
                            array->Add(item);

                            // Skip whitespace
                            while (pos < json.size() && std::isspace(json[pos]))
                                pos++;

                            if (pos >= json.size())
                                throw std::runtime_error("Unexpected end of JSON");

                            if (json[pos] == ']')
                            {
                                pos++; // Skip closing bracket
                                break;
                            }

                            if (json[pos] != ',')
                                throw std::runtime_error("Invalid JSON: expected ',' or ']'");

                            pos++; // Skip comma
                        }

                        return array;
                    }
                    else if (c == '{')
                    {
                        // Parse object
                        pos++; // Skip opening brace
                        auto map = vm::StackItem::CreateMap();

                        // Skip whitespace
                        while (pos < json.size() && std::isspace(json[pos]))
                            pos++;

                        if (pos < json.size() && json[pos] == '}')
                        {
                            pos++; // Skip closing brace
                            return map;
                        }

                        while (true)
                        {
                            // Parse key
                            if (pos >= json.size() || json[pos] != '"')
                                throw std::runtime_error("Invalid JSON: expected '\"'");

                            auto key = parseJson(json, pos);

                            // Skip whitespace
                            while (pos < json.size() && std::isspace(json[pos]))
                                pos++;

                            if (pos >= json.size() || json[pos] != ':')
                                throw std::runtime_error("Invalid JSON: expected ':'");

                            pos++; // Skip colon

                            // Parse value
                            auto value = parseJson(json, pos);
                            auto mapItem = std::dynamic_pointer_cast<vm::MapItem>(map);
                            if (mapItem) {
                                mapItem->Set(key, value);
                            }

                            // Skip whitespace
                            while (pos < json.size() && std::isspace(json[pos]))
                                pos++;

                            if (pos >= json.size())
                                throw std::runtime_error("Unexpected end of JSON");

                            if (json[pos] == '}')
                            {
                                pos++; // Skip closing brace
                                break;
                            }

                            if (json[pos] != ',')
                                throw std::runtime_error("Invalid JSON: expected ',' or '}'");

                            pos++; // Skip comma
                        }

                        return map;
                    }
                    else if (c == '-' || (c >= '0' && c <= '9'))
                    {
                        // Parse number
                        size_t start = pos;
                        if (c == '-')
                            pos++;

                        while (pos < json.size() && json[pos] >= '0' && json[pos] <= '9')
                            pos++;

                        if (pos < json.size() && json[pos] == '.')
                        {
                            pos++;
                            while (pos < json.size() && json[pos] >= '0' && json[pos] <= '9')
                                pos++;
                        }

                        if (pos < json.size() && (json[pos] == 'e' || json[pos] == 'E'))
                        {
                            pos++;
                            if (pos < json.size() && (json[pos] == '+' || json[pos] == '-'))
                                pos++;

                            while (pos < json.size() && json[pos] >= '0' && json[pos] <= '9')
                                pos++;
                        }

                        std::string numStr = json.substr(start, pos - start);
                        try
                        {
                            int64_t num = std::stoll(numStr);
                            return vm::StackItem::Create(num);
                        }
                        catch (const std::exception&)
                        {
                            throw std::runtime_error("Invalid JSON: number out of range");
                        }
                    }
                    else
                    {
                        throw std::runtime_error("Invalid JSON: unexpected character");
                    }
                };

                try
                {
                    size_t pos = 0;
                    auto result = parseJson(json, pos);

                    // Skip trailing whitespace
                    while (pos < json.size() && std::isspace(json[pos]))
                        pos++;

                    if (pos < json.size())
                        throw std::runtime_error("Invalid JSON: unexpected trailing characters");

                    context.Push(result);
                }
                catch (const std::exception& ex)
                {
                    // Return null on error
                    context.Push(vm::StackItem::Create(nullptr));
                }

                return true;
            }, 1 << 15);
        }
    }

    // This function will be called from the RegisterSystemCalls method in application_engine_system_calls.cpp
    void RegisterJsonSystemCalls(ApplicationEngine& engine)
    {
        RegisterJsonSystemCallsImpl(engine);
    }
}
