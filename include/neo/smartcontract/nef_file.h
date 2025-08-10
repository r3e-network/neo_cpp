#pragma once

#include <neo/io/byte_vector.h>
#include <neo/io/ijson_serializable.h>
#include <neo/io/iserializable.h>
#include <neo/io/uint160.h>
#include <neo/smartcontract/method_token.h>

#include <cstdint>
#include <string>
#include <vector>

namespace neo::smartcontract
{
/**
 * @brief Represents the structure of NEO Executable Format.
 */
class NefFile : public io::ISerializable, public io::IJsonSerializable
{
   public:
    /**
     * @brief NEO Executable Format 3 (NEF3)
     */
    static constexpr uint32_t Magic = 0x3346454E;

    /**
     * @brief Constructs an empty NefFile.
     */
    NefFile();

    /**
     * @brief Gets the compiler name and version.
     * @return The compiler name and version.
     */
    const std::string& GetCompiler() const;

    /**
     * @brief Sets the compiler name and version.
     * @param compiler The compiler name and version.
     */
    void SetCompiler(const std::string& compiler);

    /**
     * @brief Gets the source URL.
     * @return The source URL.
     */
    const std::string& GetSource() const;

    /**
     * @brief Sets the source URL.
     * @param source The source URL.
     */
    void SetSource(const std::string& source);

    /**
     * @brief Gets the method tokens.
     * @return The method tokens.
     */
    const std::vector<MethodToken>& GetTokens() const;

    /**
     * @brief Sets the method tokens.
     * @param tokens The method tokens.
     */
    void SetTokens(const std::vector<MethodToken>& tokens);

    /**
     * @brief Gets the script.
     * @return The script.
     */
    const io::ByteVector& GetScript() const;

    /**
     * @brief Sets the script.
     * @param script The script.
     */
    void SetScript(const io::ByteVector& script);

    /**
     * @brief Gets the checksum.
     * @return The checksum.
     */
    uint32_t GetCheckSum() const;

    /**
     * @brief Sets the checksum.
     * @param checkSum The checksum.
     */
    void SetCheckSum(uint32_t checkSum);

    /**
     * @brief Computes the checksum for this NefFile.
     * @return The computed checksum.
     */
    uint32_t ComputeChecksum() const;

    /**
     * @brief Serializes the NefFile to a binary writer.
     * @param writer The binary writer.
     */
    void Serialize(io::BinaryWriter& writer) const override;

    /**
     * @brief Deserializes the NefFile from a binary reader.
     * @param reader The binary reader.
     */
    void Deserialize(io::BinaryReader& reader) override;

    /**
     * @brief Serializes the NefFile to a JSON writer.
     * @param writer The JSON writer.
     */
    void SerializeJson(io::JsonWriter& writer) const override;

    /**
     * @brief Deserializes the NefFile from a JSON reader.
     * @param reader The JSON reader.
     */
    void DeserializeJson(const io::JsonReader& reader) override;

   private:
    std::string compiler_;
    std::string source_;
    std::vector<MethodToken> tokens_;
    io::ByteVector script_;
    uint32_t checkSum_;
};
}  // namespace neo::smartcontract
