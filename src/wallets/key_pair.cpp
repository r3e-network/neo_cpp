// Copyright (C) 2015-2025 The Neo Project.
//
// key_pair_proper.cpp file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#include <algorithm>
#include <cstring>
#include <memory>
#include <neo/cryptography/base58.h>
#include <neo/cryptography/crypto.h>
#include <neo/cryptography/ecc/secp256r1.h>
#include <neo/wallets/helper.h>
#include <neo/wallets/key_pair.h>
#include <openssl/rand.h>
#include <stdexcept>

namespace neo::wallets
{
KeyPair::KeyPair(const io::ByteVector& privateKey) : privateKey_(privateKey)
{
    if (privateKey_.size() != 32)
    {
        throw std::invalid_argument("Private key must be 32 bytes");
    }

    if (!cryptography::ecc::Secp256r1::IsValidPrivateKey(privateKey_))
    {
        throw std::invalid_argument("Invalid private key for secp256r1 curve");
    }
}

KeyPair::~KeyPair()
{
    Clear();
}

KeyPair::KeyPair(const KeyPair& other) : privateKey_(other.privateKey_)
{
    // Public key and script hash will be computed lazily
}

KeyPair::KeyPair(KeyPair&& other) noexcept
    : privateKey_(std::move(other.privateKey_)), publicKey_(std::move(other.publicKey_)),
      scriptHash_(std::move(other.scriptHash_))
{
    other.Clear();
}

KeyPair& KeyPair::operator=(const KeyPair& other)
{
    if (this != &other)
    {
        Clear();
        privateKey_ = other.privateKey_;
        // Public key and script hash will be computed lazily
    }
    return *this;
}

KeyPair& KeyPair::operator=(KeyPair&& other) noexcept
{
    if (this != &other)
    {
        Clear();
        privateKey_ = std::move(other.privateKey_);
        publicKey_ = std::move(other.publicKey_);
        scriptHash_ = std::move(other.scriptHash_);
        other.Clear();
    }
    return *this;
}

const io::ByteVector& KeyPair::GetPrivateKey() const
{
    return privateKey_;
}

const cryptography::ecc::ECPoint& KeyPair::GetPublicKey() const
{
    if (!publicKey_)
    {
        ComputePublicKey();
    }
    return *publicKey_;
}

io::UInt160 KeyPair::GetScriptHash() const
{
    if (!scriptHash_)
    {
        ComputeScriptHash();
    }
    return *scriptHash_;
}

std::string KeyPair::GetAddress(uint8_t address_version) const
{
    return Helper::ToAddress(GetScriptHash(), address_version);
}

io::ByteVector KeyPair::Sign(const io::ByteVector& data) const
{
    // Use proper ECDSA signature via Secp256r1
    return cryptography::ecc::Secp256r1::Sign(data, privateKey_);
}

bool KeyPair::Verify(const io::ByteVector& data, const io::ByteVector& signature) const
{
    // Get public key bytes for verification
    auto pubKeyBytes = GetPublicKey().ToBytes();

    // Use proper ECDSA verification via Secp256r1
    return cryptography::ecc::Secp256r1::Verify(data, signature, pubKeyBytes);
}

std::string KeyPair::ToWIF() const
{
    // Use proper WIF encoding via Secp256r1
    return cryptography::ecc::Secp256r1::PrivateKeyToWIF(privateKey_, true);
}

std::unique_ptr<KeyPair> KeyPair::Generate()
{
    // Generate cryptographically secure random private key
    io::ByteVector privateKey(32);

    // Use OpenSSL's cryptographically secure random number generator
    if (RAND_bytes(privateKey.data(), 32) != 1)
    {
        throw std::runtime_error("Failed to generate random bytes");
    }

    // Ensure the key is valid for secp256r1
    while (!cryptography::ecc::Secp256r1::IsValidPrivateKey(privateKey))
    {
        if (RAND_bytes(privateKey.data(), 32) != 1)
        {
            throw std::runtime_error("Failed to generate random bytes");
        }
    }

    return std::make_unique<KeyPair>(privateKey);
}

std::unique_ptr<KeyPair> KeyPair::FromWIF(const std::string& wif)
{
    // Use proper WIF decoding via Secp256r1
    auto privateKey = cryptography::ecc::Secp256r1::WIFToPrivateKey(wif);
    return std::make_unique<KeyPair>(privateKey);
}

KeyPair KeyPair::FromHex(const std::string& hex)
{
    auto private_key = Helper::FromHexString(hex);
    return KeyPair(private_key);
}

std::string KeyPair::ToHex() const
{
    return Helper::ToHexString(privateKey_.AsSpan());
}

bool KeyPair::IsValid() const
{
    return cryptography::ecc::Secp256r1::IsValidPrivateKey(privateKey_);
}

bool KeyPair::operator==(const KeyPair& other) const
{
    return privateKey_ == other.privateKey_;
}

bool KeyPair::operator!=(const KeyPair& other) const
{
    return !(*this == other);
}

bool KeyPair::ValidatePrivateKey(const io::ByteVector& private_key)
{
    return cryptography::ecc::Secp256r1::IsValidPrivateKey(private_key);
}

void KeyPair::ComputePublicKey() const
{
    // Use proper ECC public key computation via Secp256r1
    auto pubKeyBytes = cryptography::ecc::Secp256r1::ComputePublicKey(privateKey_);

    // Create ECPoint from the computed public key bytes
    auto ecPoint = cryptography::ecc::ECPoint::FromBytes(pubKeyBytes.AsSpan());
    publicKey_ = std::make_unique<cryptography::ecc::ECPoint>(std::move(ecPoint));
}

void KeyPair::ComputeScriptHash() const
{
    scriptHash_ = std::make_unique<io::UInt160>(Helper::GetScriptHash(GetPublicKey()));
}

void KeyPair::Clear()
{
    // Clear sensitive data
    std::fill(privateKey_.begin(), privateKey_.end(), 0);
    publicKey_.reset();
    scriptHash_.reset();
}

bool KeyPair::IsValidPrivateKey(const io::ByteVector& privateKey)
{
    return cryptography::ecc::Secp256r1::IsValidPrivateKey(privateKey);
}

std::string KeyPair::Base58Encode(const io::ByteVector& data)
{
    // Use proper Base58 encoding from cryptography library
    return cryptography::Base58::Encode(data.AsSpan());
}
}  // namespace neo::wallets