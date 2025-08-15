#include <neo/sdk/core/types.h>
#include <neo/io/binary_writer.h>
#include <neo/cryptography/crypto.h>

namespace neo::sdk::core {

// ContractParameter implementations
ContractParameter ContractParameter::FromInteger(int64_t value) {
    ContractParameter param;
    param.type = INTEGER;
    
    // Convert integer to bytes (little-endian)
    param.value.resize(sizeof(int64_t));
    std::memcpy(param.value.data(), &value, sizeof(int64_t));
    
    return param;
}

ContractParameter ContractParameter::FromString(const std::string& value) {
    ContractParameter param;
    param.type = STRING;
    param.value.assign(value.begin(), value.end());
    return param;
}

ContractParameter ContractParameter::FromAddress(const std::string& address) {
    ContractParameter param;
    param.type = HASH160;
    
    // Convert address to UInt160
    auto hash = UInt160::FromAddress(address);
    param.value = hash.ToArray();
    
    return param;
}

ContractParameter ContractParameter::FromHash160(const UInt160& hash) {
    ContractParameter param;
    param.type = HASH160;
    param.value = hash.ToArray();
    return param;
}

ContractParameter ContractParameter::FromHash256(const UInt256& hash) {
    ContractParameter param;
    param.type = HASH256;
    param.value = hash.ToArray();
    return param;
}

ContractParameter ContractParameter::FromBoolean(bool value) {
    ContractParameter param;
    param.type = BOOLEAN;
    param.value.push_back(value ? 0x01 : 0x00);
    return param;
}

ContractParameter ContractParameter::FromByteArray(const std::vector<uint8_t>& value) {
    ContractParameter param;
    param.type = BYTE_ARRAY;
    param.value = value;
    return param;
}

ContractParameter ContractParameter::Null() {
    ContractParameter param;
    param.type = VOID;
    return param;
}

} // namespace neo::sdk::core