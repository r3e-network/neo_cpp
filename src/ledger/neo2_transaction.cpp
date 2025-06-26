// Copyright (C) 2015-2025 The Neo Project.
//
// neo2_transaction.cpp file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#include <neo/ledger/neo2_transaction.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/json_writer.h>
#include <neo/cryptography/hash.h>
#include <sstream>

namespace neo::ledger {

Neo2Transaction::Neo2Transaction()
    : type_(Type::ContractTransaction), version_(0) {
}

io::UInt256 Neo2Transaction::GetHash() const {
    std::ostringstream stream;
    io::BinaryWriter writer(stream);
    
    // Serialize the transaction without witnesses for hash calculation
    writer.Write(static_cast<uint8_t>(type_));
    writer.Write(version_);
    
    // Serialize attributes
    writer.WriteVarInt(attributes_.size());
    for (const auto& attr : attributes_) {
        attr.Serialize(writer);
    }
    
    // Serialize inputs
    writer.WriteVarInt(inputs_.size());
    for (const auto& input : inputs_) {
        input.Serialize(writer);
    }
    
    // Serialize outputs
    writer.WriteVarInt(outputs_.size());
    for (const auto& output : outputs_) {
        output.Serialize(writer);
    }
    
    std::string data = stream.str();
    return cryptography::Hash::Sha256(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
}

int Neo2Transaction::GetSize() const {
    int size = 0;
    size += sizeof(uint8_t); // type
    size += sizeof(uint8_t); // version
    
    // Attributes
    size += io::BinaryWriter::GetVarIntSize(attributes_.size());
    for (const auto& attr : attributes_) {
        size += attr.GetSize();
    }
    
    // Inputs
    size += io::BinaryWriter::GetVarIntSize(inputs_.size());
    for (const auto& input : inputs_) {
        size += input.GetSize();
    }
    
    // Outputs
    size += io::BinaryWriter::GetVarIntSize(outputs_.size());
    for (const auto& output : outputs_) {
        size += output.GetSize();
    }
    
    // Witnesses
    size += io::BinaryWriter::GetVarIntSize(witnesses_.size());
    for (const auto& witness : witnesses_) {
        size += witness.GetSize();
    }
    
    return size;
}

void Neo2Transaction::Serialize(io::BinaryWriter& writer) const {
    writer.Write(static_cast<uint8_t>(type_));
    writer.Write(version_);
    
    // Serialize attributes
    writer.WriteVarInt(attributes_.size());
    for (const auto& attr : attributes_) {
        attr.Serialize(writer);
    }
    
    // Serialize inputs
    writer.WriteVarInt(inputs_.size());
    for (const auto& input : inputs_) {
        input.Serialize(writer);
    }
    
    // Serialize outputs
    writer.WriteVarInt(outputs_.size());
    for (const auto& output : outputs_) {
        output.Serialize(writer);
    }
    
    // Serialize witnesses
    writer.WriteVarInt(witnesses_.size());
    for (const auto& witness : witnesses_) {
        witness.Serialize(writer);
    }
}

void Neo2Transaction::Deserialize(io::BinaryReader& reader) {
    type_ = static_cast<Type>(reader.Read<uint8_t>());
    version_ = reader.Read<uint8_t>();
    
    // Deserialize attributes
    size_t attrCount = reader.ReadVarInt();
    attributes_.clear();
    attributes_.reserve(attrCount);
    for (size_t i = 0; i < attrCount; i++) {
        TransactionAttribute attr;
        attr.Deserialize(reader);
        attributes_.push_back(attr);
    }
    
    // Deserialize inputs
    size_t inputCount = reader.ReadVarInt();
    inputs_.clear();
    inputs_.reserve(inputCount);
    for (size_t i = 0; i < inputCount; i++) {
        CoinReference input;
        input.Deserialize(reader);
        inputs_.push_back(input);
    }
    
    // Deserialize outputs
    size_t outputCount = reader.ReadVarInt();
    outputs_.clear();
    outputs_.reserve(outputCount);
    for (size_t i = 0; i < outputCount; i++) {
        TransactionOutput output;
        output.Deserialize(reader);
        outputs_.push_back(output);
    }
    
    // Deserialize witnesses
    size_t witnessCount = reader.ReadVarInt();
    witnesses_.clear();
    witnesses_.reserve(witnessCount);
    for (size_t i = 0; i < witnessCount; i++) {
        Witness witness;
        witness.Deserialize(reader);
        witnesses_.push_back(witness);
    }
}

void Neo2Transaction::SerializeJson(io::JsonWriter& writer) const {
    writer.WriteStartObject();
    writer.Write("type", static_cast<uint8_t>(type_));
    writer.Write("version", version_);
    
    writer.WritePropertyName("attributes");
    writer.WriteStartArray();
    for (const auto& attr : attributes_) {
        attr.SerializeJson(writer);
    }
    writer.WriteEndArray();
    
    writer.WritePropertyName("inputs");
    writer.WriteStartArray();
    for (const auto& input : inputs_) {
        input.SerializeJson(writer);
    }
    writer.WriteEndArray();
    
    writer.WritePropertyName("outputs");
    writer.WriteStartArray();
    for (const auto& output : outputs_) {
        output.SerializeJson(writer);
    }
    writer.WriteEndArray();
    
    writer.WritePropertyName("witnesses");
    writer.WriteStartArray();
    for (const auto& witness : witnesses_) {
        witness.SerializeJson(writer);
    }
    writer.WriteEndArray();
    
    writer.WriteEndObject();
}

void Neo2Transaction::DeserializeJson(const io::JsonReader& reader) {
    // Implementation would parse JSON and populate fields
    // For now, just a placeholder
}

bool Neo2Transaction::operator==(const Neo2Transaction& other) const {
    return type_ == other.type_ &&
           version_ == other.version_ &&
           attributes_ == other.attributes_ &&
           inputs_ == other.inputs_ &&
           outputs_ == other.outputs_ &&
           witnesses_ == other.witnesses_;
}

bool Neo2Transaction::operator!=(const Neo2Transaction& other) const {
    return !(*this == other);
}

} // namespace neo::ledger