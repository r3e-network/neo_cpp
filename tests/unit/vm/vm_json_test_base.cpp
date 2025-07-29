#include "vm_json_test_base.h"
#include <iostream>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/vm/compound_items.h>
#include <neo/vm/primitive_items.h>
#include <neo/vm/stack_item.h>
#include <sstream>

namespace neo::vm::tests
{
// Convert VMUTStackItem to StackItem
std::shared_ptr<StackItem> VMUTStackItem::ToStackItem() const
{
    switch (type)
    {
        case VMUTStackItemType::Boolean:
            return StackItem::Create(value.get<bool>());
        case VMUTStackItemType::Integer:
            return StackItem::Create(static_cast<int64_t>(std::stoll(value.get<std::string>())));
        case VMUTStackItemType::ByteString:
        {
            std::string hexString = value.get<std::string>();
            if (hexString.substr(0, 2) == "0x")
                hexString = hexString.substr(2);

            io::ByteVector bytes;
            for (size_t i = 0; i < hexString.length(); i += 2)
            {
                std::string byteString = hexString.substr(i, 2);
                bytes.Push(static_cast<uint8_t>(std::stoi(byteString, nullptr, 16)));
            }

            return StackItem::Create(bytes);
        }
        case VMUTStackItemType::Buffer:
        {
            std::string hexString = value.get<std::string>();
            if (hexString.substr(0, 2) == "0x")
                hexString = hexString.substr(2);

            io::ByteVector bytes;
            for (size_t i = 0; i < hexString.length(); i += 2)
            {
                std::string byteString = hexString.substr(i, 2);
                bytes.Push(static_cast<uint8_t>(std::stoi(byteString, nullptr, 16)));
            }

            return std::make_shared<BufferItem>(bytes);
        }
        case VMUTStackItemType::Array:
        {
            std::vector<std::shared_ptr<StackItem>> items;
            for (const auto& item : value)
            {
                VMUTStackItem vmutItem;
                vmutItem.type = static_cast<VMUTStackItemType>(item["type"].get<int>());
                vmutItem.value = item["value"];
                items.push_back(vmutItem.ToStackItem());
            }

            return std::make_shared<ArrayItem>(items);
        }
        case VMUTStackItemType::Struct:
        {
            std::vector<std::shared_ptr<StackItem>> items;
            for (const auto& item : value)
            {
                VMUTStackItem vmutItem;
                vmutItem.type = static_cast<VMUTStackItemType>(item["type"].get<int>());
                vmutItem.value = item["value"];
                items.push_back(vmutItem.ToStackItem());
            }

            return std::make_shared<StructItem>(items);
        }
        case VMUTStackItemType::Map:
        {
            auto map = std::make_shared<MapItem>();
            for (const auto& [key, val] : value.items())
            {
                VMUTStackItem keyItem;
                keyItem.type = VMUTStackItemType::ByteString;
                keyItem.value = key;

                VMUTStackItem valueItem;
                valueItem.type = static_cast<VMUTStackItemType>(val["type"].get<int>());
                valueItem.value = val["value"];

                map->Set(keyItem.ToStackItem(), valueItem.ToStackItem());
            }

            return map;
        }
        case VMUTStackItemType::Any:
            return StackItem::Null();
        default:
            throw std::runtime_error("Unsupported stack item type");
    }
}

// Convert StackItem to VMUTStackItem
VMUTStackItem VMUTStackItem::FromStackItem(const std::shared_ptr<StackItem>& item)
{
    VMUTStackItem result;

    if (item == nullptr)
    {
        result.type = VMUTStackItemType::Any;
        return result;
    }

    switch (item->GetType())
    {
        case StackItemType::Boolean:
            result.type = VMUTStackItemType::Boolean;
            result.value = item->GetBoolean();
            break;
        case StackItemType::Integer:
            result.type = VMUTStackItemType::Integer;
            result.value = std::to_string(item->GetInteger());
            break;
        case StackItemType::ByteString:
        {
            result.type = VMUTStackItemType::ByteString;
            auto bytes = item->GetByteArray();
            std::stringstream ss;
            ss << "0x";
            for (size_t i = 0; i < bytes.Size(); i++)
                ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[i]);
            result.value = ss.str();
            break;
        }
        case StackItemType::Buffer:
        {
            result.type = VMUTStackItemType::Buffer;
            auto bytes = item->GetByteArray();
            std::stringstream ss;
            ss << "0x";
            for (size_t i = 0; i < bytes.Size(); i++)
                ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[i]);
            result.value = ss.str();
            break;
        }
        case StackItemType::Array:
        {
            result.type = VMUTStackItemType::Array;
            auto array = item->GetArray();
            nlohmann::json jsonArray = nlohmann::json::array();
            for (const auto& arrayItem : array)
            {
                auto vmutItem = FromStackItem(arrayItem);
                nlohmann::json jsonItem;
                jsonItem["type"] = static_cast<int>(vmutItem.type);
                jsonItem["value"] = vmutItem.value;
                jsonArray.push_back(jsonItem);
            }
            result.value = jsonArray;
            break;
        }
        case StackItemType::Struct:
        {
            result.type = VMUTStackItemType::Struct;
            auto array = item->GetArray();
            nlohmann::json jsonArray = nlohmann::json::array();
            for (const auto& arrayItem : array)
            {
                auto vmutItem = FromStackItem(arrayItem);
                nlohmann::json jsonItem;
                jsonItem["type"] = static_cast<int>(vmutItem.type);
                jsonItem["value"] = vmutItem.value;
                jsonArray.push_back(jsonItem);
            }
            result.value = jsonArray;
            break;
        }
        case StackItemType::Map:
        {
            result.type = VMUTStackItemType::Map;
            auto map = item->GetMap();
            nlohmann::json jsonMap = nlohmann::json::object();
            for (const auto& [key, val] : map)
            {
                auto keyItem = FromStackItem(key);
                auto valueItem = FromStackItem(val);

                nlohmann::json jsonItem;
                jsonItem["type"] = static_cast<int>(valueItem.type);
                jsonItem["value"] = valueItem.value;

                jsonMap[keyItem.value.get<std::string>()] = jsonItem;
            }
            result.value = jsonMap;
            break;
        }
        case StackItemType::Any:
            result.type = VMUTStackItemType::Any;
            break;
        default:
            throw std::runtime_error("Unsupported stack item type");
    }

    return result;
}

// Compare VMUTExecutionContextState with actual execution context
bool VMUTExecutionContextState::Equals(const ExecutionContext& context) const
{
    // Check instruction pointer
    if (instructionPointer != context.GetInstructionPointer())
        return false;

    // Check evaluation stack
    if (evaluationStack.size() != context.GetEvaluationStack().size())
        return false;

    for (size_t i = 0; i < evaluationStack.size(); i++)
    {
        auto expected = evaluationStack[i].ToStackItem();
        auto actual = context.GetEvaluationStack()[i];

        if (!expected->Equals(*actual))
            return false;
    }

    // Check static fields
    if (staticFields.size() != context.GetStaticFields().size())
        return false;

    for (size_t i = 0; i < staticFields.size(); i++)
    {
        auto expected = staticFields[i].ToStackItem();
        auto actual = context.GetStaticFields()[i];

        if (!expected->Equals(*actual))
            return false;
    }

    // Check local variables
    if (localVariables.size() != context.GetLocalVariables().size())
        return false;

    for (size_t i = 0; i < localVariables.size(); i++)
    {
        auto expected = localVariables[i].ToStackItem();
        auto actual = context.GetLocalVariables()[i];

        if (!expected->Equals(*actual))
            return false;
    }

    // Check arguments
    if (arguments.size() != context.GetArguments().size())
        return false;

    for (size_t i = 0; i < arguments.size(); i++)
    {
        auto expected = arguments[i].ToStackItem();
        auto actual = context.GetArguments()[i];

        if (!expected->Equals(*actual))
            return false;
    }

    // Check script - convert to io::ByteVector for comparison
    const auto& vmScript = context.GetScript().GetScript();
    io::ByteVector scriptCopy(vmScript.Data(), vmScript.Size());
    if (script != scriptCopy)
        return false;

    return true;
}

// Compare VMUTExecutionEngineState with actual execution engine
bool VMUTExecutionEngineState::Equals(const ExecutionEngine& engine) const
{
    // Check state
    if (state != engine.GetState())
        return false;

    // Check result stack
    if (resultStack.size() != engine.GetResultStack().size())
        return false;

    for (size_t i = 0; i < resultStack.size(); i++)
    {
        auto expected = resultStack[i].ToStackItem();
        auto actual = engine.GetResultStack()[i];

        if (!expected->Equals(*actual))
            return false;
    }

    // Check invocation stack
    if (invocationStack.size() != engine.GetInvocationStack().size())
        return false;

    for (size_t i = 0; i < invocationStack.size(); i++)
    {
        if (!invocationStack[i].Equals(*engine.GetInvocationStack()[i]))
            return false;
    }

    return true;
}

// Execute step
void VMUTStep::Execute(ExecutionEngine& engine, Debugger& debugger) const
{
    for (const auto& action : actions)
    {
        switch (action)
        {
            case VMUTActionType::Execute:
                debugger.Execute();
                break;
            case VMUTActionType::StepInto:
                debugger.StepInto();
                break;
            case VMUTActionType::StepOut:
                debugger.StepOut();
                break;
            case VMUTActionType::StepOver:
                debugger.StepOver();
                break;
        }
    }
}

// Execute test
void VMUTTest::Execute() const
{
    ExecutionEngine engine;
    Debugger debugger(engine);

    if (!script.IsEmpty())
    {
        // Convert io::ByteVector to internal::ByteVector
        internal::ByteVector internalBytes;
        internalBytes.Reserve(script.Size());
        for (size_t i = 0; i < script.Size(); ++i)
        {
            internalBytes.Push(script[i]);
        }
        Script scriptObj(internalBytes);
        engine.LoadScript(scriptObj);
    }

    for (const auto& step : steps)
    {
        step.Execute(engine, debugger);
    }
}

// Load VMUT from JSON file
VMUT VMUT::LoadFromFile(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
        throw std::runtime_error("Failed to open file: " + filePath);

    nlohmann::json json;
    file >> json;

    VMUT result;
    result.category = json["category"].get<std::string>();
    result.name = json["name"].get<std::string>();

    for (const auto& testJson : json["tests"])
    {
        VMUTTest test;
        test.name = testJson["name"].get<std::string>();

        // Parse script
        if (testJson.contains("script"))
        {
            if (testJson["script"].is_string())
            {
                std::string scriptHex = testJson["script"].get<std::string>();
                if (scriptHex.substr(0, 2) == "0x")
                    scriptHex = scriptHex.substr(2);

                for (size_t i = 0; i < scriptHex.length(); i += 2)
                {
                    std::string byteString = scriptHex.substr(i, 2);
                    test.script.Push(static_cast<uint8_t>(std::stoi(byteString, nullptr, 16)));
                }
            }
            else if (testJson["script"].is_array())
            {
                for (const auto& opcode : testJson["script"])
                {
                    if (opcode.is_string())
                    {
                        std::string opcodeStr = opcode.get<std::string>();
                        // Convert opcode string to byte
                        if (opcodeStr == "PUSH0")
                            test.script.Push(static_cast<uint8_t>(OpCode::PUSH0));
                        else if (opcodeStr == "PUSHM1")
                            test.script.Push(static_cast<uint8_t>(OpCode::PUSHM1));
                        else if (opcodeStr == "PUSH1")
                            test.script.Push(static_cast<uint8_t>(OpCode::PUSH1));
                        else if (opcodeStr == "PUSH2")
                            test.script.Push(static_cast<uint8_t>(OpCode::PUSH2));
                        else if (opcodeStr == "PUSH3")
                            test.script.Push(static_cast<uint8_t>(OpCode::PUSH3));
                        else if (opcodeStr == "PUSH4")
                            test.script.Push(static_cast<uint8_t>(OpCode::PUSH4));
                        else if (opcodeStr == "PUSH5")
                            test.script.Push(static_cast<uint8_t>(OpCode::PUSH5));
                        else if (opcodeStr == "PUSH6")
                            test.script.Push(static_cast<uint8_t>(OpCode::PUSH6));
                        else if (opcodeStr == "PUSH7")
                            test.script.Push(static_cast<uint8_t>(OpCode::PUSH7));
                        else if (opcodeStr == "PUSH8")
                            test.script.Push(static_cast<uint8_t>(OpCode::PUSH8));
                        else if (opcodeStr == "PUSH9")
                            test.script.Push(static_cast<uint8_t>(OpCode::PUSH9));
                        else if (opcodeStr == "PUSH10")
                            test.script.Push(static_cast<uint8_t>(OpCode::PUSH10));
                        else if (opcodeStr == "PUSH11")
                            test.script.Push(static_cast<uint8_t>(OpCode::PUSH11));
                        else if (opcodeStr == "PUSH12")
                            test.script.Push(static_cast<uint8_t>(OpCode::PUSH12));
                        else if (opcodeStr == "PUSH13")
                            test.script.Push(static_cast<uint8_t>(OpCode::PUSH13));
                        else if (opcodeStr == "PUSH14")
                            test.script.Push(static_cast<uint8_t>(OpCode::PUSH14));
                        else if (opcodeStr == "PUSH15")
                            test.script.Push(static_cast<uint8_t>(OpCode::PUSH15));
                        else if (opcodeStr == "PUSH16")
                            test.script.Push(static_cast<uint8_t>(OpCode::PUSH16));
                        else if (opcodeStr == "NOP")
                            test.script.Push(static_cast<uint8_t>(OpCode::NOP));
                        else if (opcodeStr == "JMP")
                            test.script.Push(static_cast<uint8_t>(OpCode::JMP));
                        else if (opcodeStr == "JMP_L")
                            test.script.Push(static_cast<uint8_t>(OpCode::JMP_L));
                        else if (opcodeStr == "JMPIF")
                            test.script.Push(static_cast<uint8_t>(OpCode::JMPIF));
                        else if (opcodeStr == "JMPIF_L")
                            test.script.Push(static_cast<uint8_t>(OpCode::JMPIF_L));
                        else if (opcodeStr == "JMPIFNOT")
                            test.script.Push(static_cast<uint8_t>(OpCode::JMPIFNOT));
                        else if (opcodeStr == "JMPIFNOT_L")
                            test.script.Push(static_cast<uint8_t>(OpCode::JMPIFNOT_L));
                        else if (opcodeStr == "JMPEQ")
                            test.script.Push(static_cast<uint8_t>(OpCode::JMPEQ));
                        else if (opcodeStr == "JMPEQ_L")
                            test.script.Push(static_cast<uint8_t>(OpCode::JMPEQ_L));
                        else if (opcodeStr == "JMPNE")
                            test.script.Push(static_cast<uint8_t>(OpCode::JMPNE));
                        else if (opcodeStr == "JMPNE_L")
                            test.script.Push(static_cast<uint8_t>(OpCode::JMPNE_L));
                        else if (opcodeStr == "JMPGT")
                            test.script.Push(static_cast<uint8_t>(OpCode::JMPGT));
                        else if (opcodeStr == "JMPGT_L")
                            test.script.Push(static_cast<uint8_t>(OpCode::JMPGT_L));
                        else if (opcodeStr == "JMPGE")
                            test.script.Push(static_cast<uint8_t>(OpCode::JMPGE));
                        else if (opcodeStr == "JMPGE_L")
                            test.script.Push(static_cast<uint8_t>(OpCode::JMPGE_L));
                        else if (opcodeStr == "JMPLT")
                            test.script.Push(static_cast<uint8_t>(OpCode::JMPLT));
                        else if (opcodeStr == "JMPLT_L")
                            test.script.Push(static_cast<uint8_t>(OpCode::JMPLT_L));
                        else if (opcodeStr == "JMPLE")
                            test.script.Push(static_cast<uint8_t>(OpCode::JMPLE));
                        else if (opcodeStr == "JMPLE_L")
                            test.script.Push(static_cast<uint8_t>(OpCode::JMPLE_L));
                        else if (opcodeStr == "CALL")
                            test.script.Push(static_cast<uint8_t>(OpCode::CALL));
                        else if (opcodeStr == "CALL_L")
                            test.script.Push(static_cast<uint8_t>(OpCode::CALL_L));
                        else if (opcodeStr == "RET")
                            test.script.Push(static_cast<uint8_t>(OpCode::RET));
                        else if (opcodeStr == "SYSCALL")
                            test.script.Push(static_cast<uint8_t>(OpCode::SYSCALL));
                        else if (opcodeStr == "DEPTH")
                            test.script.Push(static_cast<uint8_t>(OpCode::DEPTH));
                        else if (opcodeStr == "DROP")
                            test.script.Push(static_cast<uint8_t>(OpCode::DROP));
                        else if (opcodeStr == "NIP")
                            test.script.Push(static_cast<uint8_t>(OpCode::NIP));
                        else if (opcodeStr == "XDROP")
                            test.script.Push(static_cast<uint8_t>(OpCode::XDROP));
                        else if (opcodeStr == "CLEAR")
                            test.script.Push(static_cast<uint8_t>(OpCode::CLEAR));
                        else if (opcodeStr == "DUP")
                            test.script.Push(static_cast<uint8_t>(OpCode::DUP));
                        else if (opcodeStr == "OVER")
                            test.script.Push(static_cast<uint8_t>(OpCode::OVER));
                        else if (opcodeStr == "PICK")
                            test.script.Push(static_cast<uint8_t>(OpCode::PICK));
                        else if (opcodeStr == "TUCK")
                            test.script.Push(static_cast<uint8_t>(OpCode::TUCK));
                        else if (opcodeStr == "SWAP")
                            test.script.Push(static_cast<uint8_t>(OpCode::SWAP));
                        else if (opcodeStr == "ROT")
                            test.script.Push(static_cast<uint8_t>(OpCode::ROT));
                        else if (opcodeStr == "ROLL")
                            test.script.Push(static_cast<uint8_t>(OpCode::ROLL));
                        else if (opcodeStr == "REVERSE3")
                            test.script.Push(static_cast<uint8_t>(OpCode::REVERSE3));
                        else if (opcodeStr == "REVERSE4")
                            test.script.Push(static_cast<uint8_t>(OpCode::REVERSE4));
                        else if (opcodeStr == "REVERSEN")
                            test.script.Push(static_cast<uint8_t>(OpCode::REVERSEN));
                        else if (opcodeStr == "INITSSLOT")
                            test.script.Push(static_cast<uint8_t>(OpCode::INITSSLOT));
                        else if (opcodeStr == "INITSLOT")
                            test.script.Push(static_cast<uint8_t>(OpCode::INITSLOT));
                        else if (opcodeStr == "LDSFLD0")
                            test.script.Push(static_cast<uint8_t>(OpCode::LDSFLD0));
                        else if (opcodeStr == "LDSFLD1")
                            test.script.Push(static_cast<uint8_t>(OpCode::LDSFLD1));
                        else if (opcodeStr == "LDSFLD2")
                            test.script.Push(static_cast<uint8_t>(OpCode::LDSFLD2));
                        else if (opcodeStr == "LDSFLD3")
                            test.script.Push(static_cast<uint8_t>(OpCode::LDSFLD3));
                        else if (opcodeStr == "LDSFLD4")
                            test.script.Push(static_cast<uint8_t>(OpCode::LDSFLD4));
                        else if (opcodeStr == "LDSFLD5")
                            test.script.Push(static_cast<uint8_t>(OpCode::LDSFLD5));
                        else if (opcodeStr == "LDSFLD6")
                            test.script.Push(static_cast<uint8_t>(OpCode::LDSFLD6));
                        else if (opcodeStr == "LDSFLD")
                            test.script.Push(static_cast<uint8_t>(OpCode::LDSFLD));
                        else if (opcodeStr == "STSFLD0")
                            test.script.Push(static_cast<uint8_t>(OpCode::STSFLD0));
                        else if (opcodeStr == "STSFLD1")
                            test.script.Push(static_cast<uint8_t>(OpCode::STSFLD1));
                        else if (opcodeStr == "STSFLD2")
                            test.script.Push(static_cast<uint8_t>(OpCode::STSFLD2));
                        else if (opcodeStr == "STSFLD3")
                            test.script.Push(static_cast<uint8_t>(OpCode::STSFLD3));
                        else if (opcodeStr == "STSFLD4")
                            test.script.Push(static_cast<uint8_t>(OpCode::STSFLD4));
                        else if (opcodeStr == "STSFLD5")
                            test.script.Push(static_cast<uint8_t>(OpCode::STSFLD5));
                        else if (opcodeStr == "STSFLD6")
                            test.script.Push(static_cast<uint8_t>(OpCode::STSFLD6));
                        else if (opcodeStr == "STSFLD")
                            test.script.Push(static_cast<uint8_t>(OpCode::STSFLD));
                        else if (opcodeStr == "LDLOC0")
                            test.script.Push(static_cast<uint8_t>(OpCode::LDLOC0));
                        else if (opcodeStr == "LDLOC1")
                            test.script.Push(static_cast<uint8_t>(OpCode::LDLOC1));
                        else if (opcodeStr == "LDLOC2")
                            test.script.Push(static_cast<uint8_t>(OpCode::LDLOC2));
                        else if (opcodeStr == "LDLOC3")
                            test.script.Push(static_cast<uint8_t>(OpCode::LDLOC3));
                        else if (opcodeStr == "LDLOC4")
                            test.script.Push(static_cast<uint8_t>(OpCode::LDLOC4));
                        else if (opcodeStr == "LDLOC5")
                            test.script.Push(static_cast<uint8_t>(OpCode::LDLOC5));
                        else if (opcodeStr == "LDLOC6")
                            test.script.Push(static_cast<uint8_t>(OpCode::LDLOC6));
                        else if (opcodeStr == "LDLOC")
                            test.script.Push(static_cast<uint8_t>(OpCode::LDLOC));
                        else if (opcodeStr == "STLOC0")
                            test.script.Push(static_cast<uint8_t>(OpCode::STLOC0));
                        else if (opcodeStr == "STLOC1")
                            test.script.Push(static_cast<uint8_t>(OpCode::STLOC1));
                        else if (opcodeStr == "STLOC2")
                            test.script.Push(static_cast<uint8_t>(OpCode::STLOC2));
                        else if (opcodeStr == "STLOC3")
                            test.script.Push(static_cast<uint8_t>(OpCode::STLOC3));
                        else if (opcodeStr == "STLOC4")
                            test.script.Push(static_cast<uint8_t>(OpCode::STLOC4));
                        else if (opcodeStr == "STLOC5")
                            test.script.Push(static_cast<uint8_t>(OpCode::STLOC5));
                        else if (opcodeStr == "STLOC6")
                            test.script.Push(static_cast<uint8_t>(OpCode::STLOC6));
                        else if (opcodeStr == "STLOC")
                            test.script.Push(static_cast<uint8_t>(OpCode::STLOC));
                        else if (opcodeStr == "LDARG0")
                            test.script.Push(static_cast<uint8_t>(OpCode::LDARG0));
                        else if (opcodeStr == "LDARG1")
                            test.script.Push(static_cast<uint8_t>(OpCode::LDARG1));
                        else if (opcodeStr == "LDARG2")
                            test.script.Push(static_cast<uint8_t>(OpCode::LDARG2));
                        else if (opcodeStr == "LDARG3")
                            test.script.Push(static_cast<uint8_t>(OpCode::LDARG3));
                        else if (opcodeStr == "LDARG4")
                            test.script.Push(static_cast<uint8_t>(OpCode::LDARG4));
                        else if (opcodeStr == "LDARG5")
                            test.script.Push(static_cast<uint8_t>(OpCode::LDARG5));
                        else if (opcodeStr == "LDARG6")
                            test.script.Push(static_cast<uint8_t>(OpCode::LDARG6));
                        else if (opcodeStr == "LDARG")
                            test.script.Push(static_cast<uint8_t>(OpCode::LDARG));
                        else if (opcodeStr == "STARG0")
                            test.script.Push(static_cast<uint8_t>(OpCode::STARG0));
                        else if (opcodeStr == "STARG1")
                            test.script.Push(static_cast<uint8_t>(OpCode::STARG1));
                        else if (opcodeStr == "STARG2")
                            test.script.Push(static_cast<uint8_t>(OpCode::STARG2));
                        else if (opcodeStr == "STARG3")
                            test.script.Push(static_cast<uint8_t>(OpCode::STARG3));
                        else if (opcodeStr == "STARG4")
                            test.script.Push(static_cast<uint8_t>(OpCode::STARG4));
                        else if (opcodeStr == "STARG5")
                            test.script.Push(static_cast<uint8_t>(OpCode::STARG5));
                        else if (opcodeStr == "STARG6")
                            test.script.Push(static_cast<uint8_t>(OpCode::STARG6));
                        else if (opcodeStr == "STARG")
                            test.script.Push(static_cast<uint8_t>(OpCode::STARG));
                        else if (opcodeStr == "NEWBUFFER")
                            test.script.Push(static_cast<uint8_t>(OpCode::NEWBUFFER));
                        else if (opcodeStr == "MEMCPY")
                            test.script.Push(static_cast<uint8_t>(OpCode::MEMCPY));
                        else if (opcodeStr == "CAT")
                            test.script.Push(static_cast<uint8_t>(OpCode::CAT));
                        else if (opcodeStr == "SUBSTR")
                            test.script.Push(static_cast<uint8_t>(OpCode::SUBSTR));
                        else if (opcodeStr == "LEFT")
                            test.script.Push(static_cast<uint8_t>(OpCode::LEFT));
                        else if (opcodeStr == "RIGHT")
                            test.script.Push(static_cast<uint8_t>(OpCode::RIGHT));
                        else if (opcodeStr == "INVERT")
                            test.script.Push(static_cast<uint8_t>(OpCode::INVERT));
                        else if (opcodeStr == "AND")
                            test.script.Push(static_cast<uint8_t>(OpCode::AND));
                        else if (opcodeStr == "OR")
                            test.script.Push(static_cast<uint8_t>(OpCode::OR));
                        else if (opcodeStr == "XOR")
                            test.script.Push(static_cast<uint8_t>(OpCode::XOR));
                        else if (opcodeStr == "EQUAL")
                            test.script.Push(static_cast<uint8_t>(OpCode::EQUAL));
                        else if (opcodeStr == "NOTEQUAL")
                            test.script.Push(static_cast<uint8_t>(OpCode::NOTEQUAL));
                        else if (opcodeStr == "SIGN")
                            test.script.Push(static_cast<uint8_t>(OpCode::SIGN));
                        else if (opcodeStr == "ABS")
                            test.script.Push(static_cast<uint8_t>(OpCode::ABS));
                        else if (opcodeStr == "NEGATE")
                            test.script.Push(static_cast<uint8_t>(OpCode::NEGATE));
                        else if (opcodeStr == "INC")
                            test.script.Push(static_cast<uint8_t>(OpCode::INC));
                        else if (opcodeStr == "DEC")
                            test.script.Push(static_cast<uint8_t>(OpCode::DEC));
                        else if (opcodeStr == "ADD")
                            test.script.Push(static_cast<uint8_t>(OpCode::ADD));
                        else if (opcodeStr == "SUB")
                            test.script.Push(static_cast<uint8_t>(OpCode::SUB));
                        else if (opcodeStr == "MUL")
                            test.script.Push(static_cast<uint8_t>(OpCode::MUL));
                        else if (opcodeStr == "DIV")
                            test.script.Push(static_cast<uint8_t>(OpCode::DIV));
                        else if (opcodeStr == "MOD")
                            test.script.Push(static_cast<uint8_t>(OpCode::MOD));
                        else if (opcodeStr == "POW")
                            test.script.Push(static_cast<uint8_t>(OpCode::POW));
                        else if (opcodeStr == "SQRT")
                            test.script.Push(static_cast<uint8_t>(OpCode::SQRT));
                        else if (opcodeStr == "SHL")
                            test.script.Push(static_cast<uint8_t>(OpCode::SHL));
                        else if (opcodeStr == "SHR")
                            test.script.Push(static_cast<uint8_t>(OpCode::SHR));
                        else if (opcodeStr == "NOT")
                            test.script.Push(static_cast<uint8_t>(OpCode::NOT));
                        else if (opcodeStr == "BOOLAND")
                            test.script.Push(static_cast<uint8_t>(OpCode::BOOLAND));
                        else if (opcodeStr == "BOOLOR")
                            test.script.Push(static_cast<uint8_t>(OpCode::BOOLOR));
                        else if (opcodeStr == "NZ")
                            test.script.Push(static_cast<uint8_t>(OpCode::NZ));
                        else if (opcodeStr == "NUMEQUAL")
                            test.script.Push(static_cast<uint8_t>(OpCode::NUMEQUAL));
                        else if (opcodeStr == "NUMNOTEQUAL")
                            test.script.Push(static_cast<uint8_t>(OpCode::NUMNOTEQUAL));
                        else if (opcodeStr == "LT")
                            test.script.Push(static_cast<uint8_t>(OpCode::LT));
                        else if (opcodeStr == "LE")
                            test.script.Push(static_cast<uint8_t>(OpCode::LE));
                        else if (opcodeStr == "GT")
                            test.script.Push(static_cast<uint8_t>(OpCode::GT));
                        else if (opcodeStr == "GE")
                            test.script.Push(static_cast<uint8_t>(OpCode::GE));
                        else if (opcodeStr == "MIN")
                            test.script.Push(static_cast<uint8_t>(OpCode::MIN));
                        else if (opcodeStr == "MAX")
                            test.script.Push(static_cast<uint8_t>(OpCode::MAX));
                        else if (opcodeStr == "WITHIN")
                            test.script.Push(static_cast<uint8_t>(OpCode::WITHIN));
                        else if (opcodeStr == "PACKMAP")
                            test.script.Push(static_cast<uint8_t>(OpCode::PACKMAP));
                        else if (opcodeStr == "PACKSTRUCT")
                            test.script.Push(static_cast<uint8_t>(OpCode::PACKSTRUCT));
                        else if (opcodeStr == "PACK")
                            test.script.Push(static_cast<uint8_t>(OpCode::PACK));
                        else if (opcodeStr == "UNPACK")
                            test.script.Push(static_cast<uint8_t>(OpCode::UNPACK));
                        else if (opcodeStr == "NEWARRAY0")
                            test.script.Push(static_cast<uint8_t>(OpCode::NEWARRAY0));
                        else if (opcodeStr == "NEWARRAY")
                            test.script.Push(static_cast<uint8_t>(OpCode::NEWARRAY));
                        else if (opcodeStr == "NEWARRAY_T")
                            test.script.Push(static_cast<uint8_t>(OpCode::NEWARRAY_T));
                        else if (opcodeStr == "NEWSTRUCT0")
                            test.script.Push(static_cast<uint8_t>(OpCode::NEWSTRUCT0));
                        else if (opcodeStr == "NEWSTRUCT")
                            test.script.Push(static_cast<uint8_t>(OpCode::NEWSTRUCT));
                        else if (opcodeStr == "NEWMAP")
                            test.script.Push(static_cast<uint8_t>(OpCode::NEWMAP));
                        else if (opcodeStr == "SIZE")
                            test.script.Push(static_cast<uint8_t>(OpCode::SIZE));
                        else if (opcodeStr == "HASKEY")
                            test.script.Push(static_cast<uint8_t>(OpCode::HASKEY));
                        else if (opcodeStr == "KEYS")
                            test.script.Push(static_cast<uint8_t>(OpCode::KEYS));
                        else if (opcodeStr == "VALUES")
                            test.script.Push(static_cast<uint8_t>(OpCode::VALUES));
                        else if (opcodeStr == "PICKITEM")
                            test.script.Push(static_cast<uint8_t>(OpCode::PICKITEM));
                        else if (opcodeStr == "APPEND")
                            test.script.Push(static_cast<uint8_t>(OpCode::APPEND));
                        else if (opcodeStr == "SETITEM")
                            test.script.Push(static_cast<uint8_t>(OpCode::SETITEM));
                        else if (opcodeStr == "REVERSEITEMS")
                            test.script.Push(static_cast<uint8_t>(OpCode::REVERSEITEMS));
                        else if (opcodeStr == "REMOVE")
                            test.script.Push(static_cast<uint8_t>(OpCode::REMOVE));
                        else if (opcodeStr == "CLEARITEMS")
                            test.script.Push(static_cast<uint8_t>(OpCode::CLEARITEMS));
                        else if (opcodeStr == "POPITEM")
                            test.script.Push(static_cast<uint8_t>(OpCode::POPITEM));
                        else if (opcodeStr == "ISNULL")
                            test.script.Push(static_cast<uint8_t>(OpCode::ISNULL));
                        else if (opcodeStr == "ISTYPE")
                            test.script.Push(static_cast<uint8_t>(OpCode::ISTYPE));
                        else if (opcodeStr == "CONVERT")
                            test.script.Push(static_cast<uint8_t>(OpCode::CONVERT));
                        else if (opcodeStr == "ABORT")
                            test.script.Push(static_cast<uint8_t>(OpCode::ABORT));
                        else if (opcodeStr == "ASSERT")
                            test.script.Push(static_cast<uint8_t>(OpCode::ASSERT));
                        else if (opcodeStr == "THROW")
                            test.script.Push(static_cast<uint8_t>(OpCode::THROW));
                        else if (opcodeStr == "TRY")
                            test.script.Push(static_cast<uint8_t>(OpCode::TRY));
                        else if (opcodeStr == "TRY_L")
                            test.script.Push(static_cast<uint8_t>(OpCode::TRY_L));
                        else if (opcodeStr == "ENDTRY")
                            test.script.Push(static_cast<uint8_t>(OpCode::ENDTRY));
                        else if (opcodeStr == "ENDTRY_L")
                            test.script.Push(static_cast<uint8_t>(OpCode::ENDTRY_L));
                        else if (opcodeStr == "ENDFINALLY")
                            test.script.Push(static_cast<uint8_t>(OpCode::ENDFINALLY));
                        else if (opcodeStr == "PUSHA")
                            test.script.Push(static_cast<uint8_t>(OpCode::PUSHA));
                        else if (opcodeStr == "PUSHDATA1")
                            test.script.Push(static_cast<uint8_t>(OpCode::PUSHDATA1));
                        else if (opcodeStr == "PUSHDATA2")
                            test.script.Push(static_cast<uint8_t>(OpCode::PUSHDATA2));
                        else if (opcodeStr == "PUSHDATA4")
                            test.script.Push(static_cast<uint8_t>(OpCode::PUSHDATA4));
                        else
                        {
                            std::cerr << "Unknown opcode string: " << opcodeStr << std::endl;
                        }
                    }
                    else if (opcode.is_number())
                    {
                        test.script.Push(static_cast<uint8_t>(opcode.get<int>()));
                    }
                }
            }
        }

        // Parse steps
        if (testJson.contains("steps"))
        {
            for (const auto& stepJson : testJson["steps"])
            {
                VMUTStep step;

                if (stepJson.contains("name"))
                    step.name = stepJson["name"].get<std::string>();

                // Parse actions
                if (stepJson.contains("actions"))
                {
                    for (const auto& actionJson : stepJson["actions"])
                    {
                        std::string actionStr = actionJson.get<std::string>();
                        if (actionStr == "execute")
                            step.actions.push_back(VMUTActionType::Execute);
                        else if (actionStr == "stepInto")
                            step.actions.push_back(VMUTActionType::StepInto);
                        else if (actionStr == "stepOut")
                            step.actions.push_back(VMUTActionType::StepOut);
                        else if (actionStr == "stepOver")
                            step.actions.push_back(VMUTActionType::StepOver);
                    }
                }

                // Parse result
                if (stepJson.contains("result"))
                {
                    const auto& resultJson = stepJson["result"];

                    if (resultJson.contains("state"))
                    {
                        std::string stateStr = resultJson["state"].get<std::string>();
                        if (stateStr == "HALT")
                            step.result.state = VMState::Halt;
                        else if (stateStr == "BREAK")
                            step.result.state = VMState::Break;
                        else if (stateStr == "FAULT")
                            step.result.state = VMState::Fault;
                        else if (stateStr == "NONE")
                            step.result.state = VMState::None;
                    }

                    // Parse result stack
                    if (resultJson.contains("resultStack"))
                    {
                        for (const auto& itemJson : resultJson["resultStack"])
                        {
                            VMUTStackItem item;
                            item.type = static_cast<VMUTStackItemType>(itemJson["type"].get<int>());
                            item.value = itemJson["value"];
                            step.result.resultStack.push_back(item);
                        }
                    }

                    // Parse invocation stack
                    if (resultJson.contains("invocationStack"))
                    {
                        for (const auto& contextJson : resultJson["invocationStack"])
                        {
                            VMUTExecutionContextState context;

                            if (contextJson.contains("instructionPointer"))
                                context.instructionPointer = contextJson["instructionPointer"].get<int>();

                            // Parse evaluation stack
                            if (contextJson.contains("evaluationStack"))
                            {
                                for (const auto& itemJson : contextJson["evaluationStack"])
                                {
                                    VMUTStackItem item;
                                    item.type = static_cast<VMUTStackItemType>(itemJson["type"].get<int>());
                                    item.value = itemJson["value"];
                                    context.evaluationStack.push_back(item);
                                }
                            }

                            // Parse static fields
                            if (contextJson.contains("staticFields"))
                            {
                                for (const auto& itemJson : contextJson["staticFields"])
                                {
                                    VMUTStackItem item;
                                    item.type = static_cast<VMUTStackItemType>(itemJson["type"].get<int>());
                                    item.value = itemJson["value"];
                                    context.staticFields.push_back(item);
                                }
                            }

                            // Parse local variables
                            if (contextJson.contains("localVariables"))
                            {
                                for (const auto& itemJson : contextJson["localVariables"])
                                {
                                    VMUTStackItem item;
                                    item.type = static_cast<VMUTStackItemType>(itemJson["type"].get<int>());
                                    item.value = itemJson["value"];
                                    context.localVariables.push_back(item);
                                }
                            }

                            // Parse arguments
                            if (contextJson.contains("arguments"))
                            {
                                for (const auto& itemJson : contextJson["arguments"])
                                {
                                    VMUTStackItem item;
                                    item.type = static_cast<VMUTStackItemType>(itemJson["type"].get<int>());
                                    item.value = itemJson["value"];
                                    context.arguments.push_back(item);
                                }
                            }

                            // Parse script
                            if (contextJson.contains("script"))
                            {
                                std::string scriptHex = contextJson["script"].get<std::string>();
                                if (scriptHex.substr(0, 2) == "0x")
                                    scriptHex = scriptHex.substr(2);

                                for (size_t i = 0; i < scriptHex.length(); i += 2)
                                {
                                    std::string byteString = scriptHex.substr(i, 2);
                                    context.script.Push(static_cast<uint8_t>(std::stoi(byteString, nullptr, 16)));
                                }
                            }

                            step.result.invocationStack.push_back(context);
                        }
                    }

                    // Parse exception message
                    if (resultJson.contains("exceptionMessage"))
                        step.result.exceptionMessage = resultJson["exceptionMessage"].get<std::string>();
                }

                test.steps.push_back(step);
            }
        }

        result.tests.push_back(test);
    }

    return result;
}

// Execute all tests
void VMUT::Execute() const
{
    for (const auto& test : tests)
    {
        test.Execute();
    }
}

// Execute test from JSON file
void VMJsonTestBase::TestJson(const std::string& path)
{
    for (const auto& entry : std::filesystem::directory_iterator(path))
    {
        if (entry.path().extension() == ".json")
        {
            std::cout << "Processing file: " << entry.path().string() << std::endl;

            try
            {
                VMUT ut = VMUT::LoadFromFile(entry.path().string());
                ut.Execute();
            }
            catch (const std::exception& ex)
            {
                FAIL() << "Error in file: " << entry.path().string() << " - " << ex.what();
            }
        }
    }
}

// Assert result
void VMJsonTestBase::AssertResult(const VMUTExecutionEngineState& expected, const ExecutionEngine& engine,
                                  const std::string& message)
{
    EXPECT_TRUE(expected.Equals(engine)) << message;
}
}  // namespace neo::vm::tests
