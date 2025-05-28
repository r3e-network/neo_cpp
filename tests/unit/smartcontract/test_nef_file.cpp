#include <neo/smartcontract/nef_file.h>
#include <neo/smartcontract/method_token.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/cryptography/hash.h>
#include <gtest/gtest.h>
#include <sstream>

using namespace neo::smartcontract;
using namespace neo::io;

class UT_NefFile : public testing::Test
{
protected:
    void SetUp() override
    {
        // Create a sample NefFile for testing
        nefFile.SetCompiler("neo-cpp-compiler-v1.0");
        nefFile.SetSource("https://github.com/neo-project/neo-cpp");
        
        // Create a sample MethodToken
        MethodToken token;
        token.SetHash(UInt160::Parse("0xa400ff00ff00ff00ff00ff00ff00ff00ff00ff01"));
        token.SetMethod("testMethod");
        token.SetParametersCount(2);
        token.SetHasReturnValue(true);
        token.SetCallFlags(CallFlags::All);
        
        std::vector<MethodToken> tokens = { token };
        nefFile.SetTokens(tokens);
        
        // Create a sample script
        ByteVector script;
        script.Push(0x01);
        script.Push(0x02);
        script.Push(0x03);
        nefFile.SetScript(script);
        
        // Compute and set the checksum
        nefFile.SetCheckSum(nefFile.ComputeChecksum());
    }
    
    NefFile nefFile;
};

TEST_F(UT_NefFile, TestSerializeDeserialize)
{
    // Serialize the NefFile
    std::stringstream stream;
    BinaryWriter writer(stream);
    nefFile.Serialize(writer);
    
    // Get the serialized data
    std::string data = stream.str();
    
    // Deserialize the NefFile
    std::stringstream inputStream(data);
    BinaryReader reader(inputStream);
    NefFile deserializedNefFile;
    deserializedNefFile.Deserialize(reader);
    
    // Verify the deserialized NefFile
    EXPECT_EQ(nefFile.GetCompiler(), deserializedNefFile.GetCompiler());
    EXPECT_EQ(nefFile.GetSource(), deserializedNefFile.GetSource());
    EXPECT_EQ(nefFile.GetTokens().size(), deserializedNefFile.GetTokens().size());
    EXPECT_EQ(nefFile.GetScript().Size(), deserializedNefFile.GetScript().Size());
    EXPECT_EQ(nefFile.GetCheckSum(), deserializedNefFile.GetCheckSum());
    
    // Verify the token
    const auto& originalToken = nefFile.GetTokens()[0];
    const auto& deserializedToken = deserializedNefFile.GetTokens()[0];
    
    EXPECT_EQ(originalToken.GetHash(), deserializedToken.GetHash());
    EXPECT_EQ(originalToken.GetMethod(), deserializedToken.GetMethod());
    EXPECT_EQ(originalToken.GetParametersCount(), deserializedToken.GetParametersCount());
    EXPECT_EQ(originalToken.GetHasReturnValue(), deserializedToken.GetHasReturnValue());
    EXPECT_EQ(originalToken.GetCallFlags(), deserializedToken.GetCallFlags());
    
    // Verify the script
    for (size_t i = 0; i < nefFile.GetScript().Size(); i++)
    {
        EXPECT_EQ(nefFile.GetScript()[i], deserializedNefFile.GetScript()[i]);
    }
}

TEST_F(UT_NefFile, TestChecksum)
{
    // Verify that the checksum is correct
    uint32_t computedChecksum = nefFile.ComputeChecksum();
    EXPECT_EQ(nefFile.GetCheckSum(), computedChecksum);
    
    // Modify the script and verify that the checksum changes
    ByteVector modifiedScript;
    modifiedScript.Push(0x04);
    modifiedScript.Push(0x05);
    modifiedScript.Push(0x06);
    
    NefFile modifiedNefFile = nefFile;
    modifiedNefFile.SetScript(modifiedScript);
    
    uint32_t modifiedChecksum = modifiedNefFile.ComputeChecksum();
    EXPECT_NE(nefFile.GetCheckSum(), modifiedChecksum);
}

TEST_F(UT_NefFile, TestInvalidDeserialization)
{
    // Test with invalid magic
    std::stringstream stream;
    BinaryWriter writer(stream);
    
    // Write invalid magic
    writer.Write(static_cast<uint32_t>(0x12345678));
    
    // Write the rest of the data
    writer.WriteFixedString(nefFile.GetCompiler(), 64);
    writer.WriteVarString(nefFile.GetSource());
    writer.Write(static_cast<uint8_t>(0));
    writer.WriteVarInt(nefFile.GetTokens().size());
    for (const auto& token : nefFile.GetTokens())
    {
        token.Serialize(writer);
    }
    writer.Write(static_cast<uint16_t>(0));
    writer.WriteVarBytes(nefFile.GetScript());
    writer.Write(nefFile.GetCheckSum());
    
    // Try to deserialize
    std::string data = stream.str();
    std::stringstream inputStream(data);
    BinaryReader reader(inputStream);
    NefFile deserializedNefFile;
    
    EXPECT_THROW(deserializedNefFile.Deserialize(reader), std::runtime_error);
}

class UT_MethodToken : public testing::Test
{
protected:
    void SetUp() override
    {
        // Create a sample MethodToken for testing
        token.SetHash(UInt160::Parse("0xa400ff00ff00ff00ff00ff00ff00ff00ff00ff01"));
        token.SetMethod("testMethod");
        token.SetParametersCount(2);
        token.SetHasReturnValue(true);
        token.SetCallFlags(CallFlags::All);
    }
    
    MethodToken token;
};

TEST_F(UT_MethodToken, TestSerializeDeserialize)
{
    // Serialize the MethodToken
    std::stringstream stream;
    BinaryWriter writer(stream);
    token.Serialize(writer);
    
    // Get the serialized data
    std::string data = stream.str();
    
    // Deserialize the MethodToken
    std::stringstream inputStream(data);
    BinaryReader reader(inputStream);
    MethodToken deserializedToken;
    deserializedToken.Deserialize(reader);
    
    // Verify the deserialized MethodToken
    EXPECT_EQ(token.GetHash(), deserializedToken.GetHash());
    EXPECT_EQ(token.GetMethod(), deserializedToken.GetMethod());
    EXPECT_EQ(token.GetParametersCount(), deserializedToken.GetParametersCount());
    EXPECT_EQ(token.GetHasReturnValue(), deserializedToken.GetHasReturnValue());
    EXPECT_EQ(token.GetCallFlags(), deserializedToken.GetCallFlags());
}

TEST_F(UT_MethodToken, TestInvalidDeserialization)
{
    // Test with invalid method name (starting with underscore)
    std::stringstream stream;
    BinaryWriter writer(stream);
    
    // Write hash
    writer.Write(token.GetHash());
    
    // Write invalid method name
    writer.WriteVarString("_invalidMethod");
    
    // Write the rest of the data
    writer.Write(token.GetParametersCount());
    writer.Write(token.GetHasReturnValue());
    writer.Write(static_cast<uint8_t>(token.GetCallFlags()));
    
    // Try to deserialize
    std::string data = stream.str();
    std::stringstream inputStream(data);
    BinaryReader reader(inputStream);
    MethodToken deserializedToken;
    
    EXPECT_THROW(deserializedToken.Deserialize(reader), std::runtime_error);
    
    // Test with invalid call flags
    stream.str("");
    writer = BinaryWriter(stream);
    
    // Write hash
    writer.Write(token.GetHash());
    
    // Write method name
    writer.WriteVarString(token.GetMethod());
    
    // Write parameters count
    writer.Write(token.GetParametersCount());
    
    // Write has return value
    writer.Write(token.GetHasReturnValue());
    
    // Write invalid call flags
    writer.Write(static_cast<uint8_t>(255));
    
    // Try to deserialize
    data = stream.str();
    inputStream = std::stringstream(data);
    reader = BinaryReader(inputStream);
    deserializedToken = MethodToken();
    
    EXPECT_THROW(deserializedToken.Deserialize(reader), std::runtime_error);
}
