#include <neo/smartcontract/method_token.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <stdexcept>

namespace neo::smartcontract
{
    MethodToken::MethodToken()
        : parametersCount_(0), hasReturnValue_(false), callFlags_(CallFlags::None)
    {
    }

    const io::UInt160& MethodToken::GetHash() const
    {
        return hash_;
    }

    void MethodToken::SetHash(const io::UInt160& hash)
    {
        hash_ = hash;
    }

    const std::string& MethodToken::GetMethod() const
    {
        return method_;
    }

    void MethodToken::SetMethod(const std::string& method)
    {
        method_ = method;
    }

    uint16_t MethodToken::GetParametersCount() const
    {
        return parametersCount_;
    }

    void MethodToken::SetParametersCount(uint16_t parametersCount)
    {
        parametersCount_ = parametersCount;
    }

    bool MethodToken::GetHasReturnValue() const
    {
        return hasReturnValue_;
    }

    void MethodToken::SetHasReturnValue(bool hasReturnValue)
    {
        hasReturnValue_ = hasReturnValue;
    }

    CallFlags MethodToken::GetCallFlags() const
    {
        return callFlags_;
    }

    void MethodToken::SetCallFlags(CallFlags callFlags)
    {
        callFlags_ = callFlags;
    }

    void MethodToken::Serialize(io::BinaryWriter& writer) const
    {
        writer.Write(hash_);
        writer.WriteVarString(method_);
        writer.Write(parametersCount_);
        writer.Write(hasReturnValue_);
        writer.Write(static_cast<uint8_t>(callFlags_));
    }

    void MethodToken::Deserialize(io::BinaryReader& reader)
    {
        hash_ = reader.ReadUInt160();
        method_ = reader.ReadVarString();
        
        // Check if method starts with underscore
        if (!method_.empty() && method_[0] == '_')
            throw std::runtime_error("Invalid method token: method name cannot start with underscore");
        
        parametersCount_ = reader.ReadUInt16();
        hasReturnValue_ = reader.ReadBool();
        callFlags_ = static_cast<CallFlags>(reader.ReadUInt8());
        
        // Check if call flags are valid
        if ((callFlags_ & ~CallFlags::All) != CallFlags::None)
            throw std::runtime_error("Invalid method token: invalid call flags");
    }

    void MethodToken::SerializeJson(io::JsonWriter& writer) const
    {
        writer.WriteStartObject();
        
        writer.WritePropertyName("hash");
        writer.WriteString(hash_.ToString());
        
        writer.WritePropertyName("method");
        writer.WriteString(method_);
        
        writer.WritePropertyName("paramcount");
        writer.WriteNumber(parametersCount_);
        
        writer.Write("hasreturnvalue", hasReturnValue_);
        
        writer.WritePropertyName("callflags");
        writer.WriteString(std::to_string(static_cast<uint8_t>(callFlags_)));
        
        writer.WriteEndObject();
    }

    void MethodToken::DeserializeJson(const io::JsonReader& reader)
    {
        hash_ = io::UInt160::Parse(reader.ReadString("hash"));
        method_ = reader.ReadString("method");
        parametersCount_ = static_cast<uint16_t>(reader.ReadNumber("paramcount"));
        hasReturnValue_ = reader.ReadBool("hasreturnvalue");
        callFlags_ = static_cast<CallFlags>(static_cast<uint8_t>(reader.ReadNumber("callflags")));
    }
}
