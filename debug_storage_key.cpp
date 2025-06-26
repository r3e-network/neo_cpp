#include <neo/persistence/storage_key.h>
#include <iostream>
#include <iomanip>

int main() {
    auto hash1 = neo::io::UInt160::Parse("0102030405060708090a0b0c0d0e0f1011121314");
    auto hash2 = neo::io::UInt160::Parse("1102030405060708090a0b0c0d0e0f1011121314");
    auto key = neo::io::ByteVector::Parse("0102030405");
    
    neo::persistence::StorageKey storageKey1(hash1, key);
    neo::persistence::StorageKey storageKey2(hash2, key);
    
    std::cout << "StorageKey1 script hash: " << storageKey1.GetScriptHash().ToHexString() << std::endl;
    std::cout << "StorageKey2 script hash: " << storageKey2.GetScriptHash().ToHexString() << std::endl;
    
    std::cout << "Hash1 original: " << hash1.ToHexString() << std::endl;
    std::cout << "Hash2 original: " << hash2.ToHexString() << std::endl;
    
    std::cout << "Hash1 IsZero: " << hash1.IsZero() << std::endl;
    std::cout << "Hash2 IsZero: " << hash2.IsZero() << std::endl;
    
    std::cout << "StorageKey1 == StorageKey2: " << (storageKey1 == storageKey2) << std::endl;
    std::cout << "Hash1 == Hash2: " << (hash1 == hash2) << std::endl;
    
    return 0;
}
