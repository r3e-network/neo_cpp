#include <iostream>
#include <iomanip>
#include <string>
#include <cstring>
#include <cstdint>
#include <array>

struct UInt160 {
    std::array<uint8_t, 20> data_;
    
    UInt160() : data_() {}
    
    static UInt160 Parse(const std::string& hex) {
        UInt160 result;
        std::string cleanHex = hex;
        if (cleanHex.size() < 40) {
            cleanHex = std::string(40 - cleanHex.size(), '0') + cleanHex;
        }
        
        // Parse hex string (big-endian to little-endian)
        for (size_t i = 0; i < 20; ++i) {
            std::string byteStr = cleanHex.substr((20 - 1 - i) * 2, 2);
            unsigned long byteVal = std::strtoul(byteStr.c_str(), nullptr, 16);
            result.data_[i] = static_cast<uint8_t>(byteVal);
        }
        return result;
    }
    
    bool IsZero() const {
        for (size_t i = 0; i < 20; ++i) {
            if (data_[i] != 0) return false;
        }
        return true;
    }
    
    bool operator==(const UInt160& other) const {
        return data_ == other.data_;
    }
    
    const uint8_t* Data() const { return data_.data(); }
};

struct StorageKey {
    int32_t id_;
    UInt160 scriptHash_;
    
    StorageKey(const UInt160& scriptHash) : scriptHash_(scriptHash) {
        id_ = *reinterpret_cast<const int32_t*>(scriptHash.Data());
    }
    
    bool operator==(const StorageKey& other) const {
        // If both have script hashes, compare them first (more specific)
        if (!scriptHash_.IsZero() && !other.scriptHash_.IsZero()) {
            return scriptHash_ == other.scriptHash_;
        }
        
        // If only one has a script hash, they're different
        if (!scriptHash_.IsZero() || !other.scriptHash_.IsZero()) {
            return false;
        }
        
        // Neither has script hash, compare by contract ID
        return id_ == other.id_;
    }
};

int main() {
    auto hash1 = UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314");
    auto hash2 = UInt160::Parse("1102030405060708090a0b0c0d0e0f1011121314");
    
    StorageKey storageKey1(hash1);
    StorageKey storageKey2(hash2);
    
    std::cout << "Hash1 bytes: ";
    for (int i = 0; i < 20; i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)hash1.Data()[i] << " ";
    }
    std::cout << std::endl;
    
    std::cout << "Hash2 bytes: ";
    for (int i = 0; i < 20; i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)hash2.Data()[i] << " ";
    }
    std::cout << std::endl;
    
    std::cout << "Hash1 IsZero: " << hash1.IsZero() << std::endl;
    std::cout << "Hash2 IsZero: " << hash2.IsZero() << std::endl;
    std::cout << "Hash1 == Hash2: " << (hash1 == hash2) << std::endl;
    std::cout << "StorageKey1 == StorageKey2: " << (storageKey1 == storageKey2) << std::endl;
    
    return 0;
}
