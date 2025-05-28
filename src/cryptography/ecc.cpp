#include <neo/cryptography/ecc.h>
#include <neo/cryptography/hash.h>
#include <neo/io/byte_vector.h>
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>
#include <openssl/bn.h>
#include <openssl/err.h>
#include <stdexcept>
#include <unordered_map>

namespace neo::cryptography
{
    class ECPoint::Impl
    {
    public:
        EC_KEY* key;
        std::string curveName;

        Impl(EC_KEY* key, const std::string& curveName)
            : key(key), curveName(curveName)
        {
        }

        ~Impl()
        {
            EC_KEY_free(key);
        }

        Impl(const Impl& other) = delete;
        Impl& operator=(const Impl& other) = delete;
    };

    ECPoint::ECPoint(std::unique_ptr<Impl> impl)
        : impl_(std::move(impl))
    {
    }

    ECPoint::~ECPoint() = default;

    ECPoint ECPoint::FromBytes(const io::ByteSpan& data, const std::string& curve)
    {
        int nid;
        if (curve == "secp256r1")
            nid = NID_X9_62_prime256v1;
        else if (curve == "secp256k1")
            nid = NID_secp256k1;
        else
            throw std::invalid_argument("Invalid curve name");

        EC_KEY* key = EC_KEY_new_by_curve_name(nid);
        if (!key)
            throw std::runtime_error("Failed to create EC_KEY");

        const EC_GROUP* group = EC_KEY_get0_group(key);
        EC_POINT* point = EC_POINT_new(group);

        if (!EC_POINT_oct2point(group, point, data.Data(), data.Size(), nullptr))
        {
            EC_POINT_free(point);
            EC_KEY_free(key);
            throw std::invalid_argument("Invalid ECPoint data");
        }

        if (!EC_KEY_set_public_key(key, point))
        {
            EC_POINT_free(point);
            EC_KEY_free(key);
            throw std::runtime_error("Failed to set public key");
        }

        EC_POINT_free(point);

        return ECPoint(std::make_unique<Impl>(key, curve));
    }

    ECPoint ECPoint::FromHex(const std::string& hex, const std::string& curve)
    {
        io::ByteVector data = io::ByteVector::Parse(hex);
        return FromBytes(data.AsSpan(), curve);
    }

    io::ByteVector ECPoint::ToBytes(bool compressed) const
    {
        const EC_GROUP* group = EC_KEY_get0_group(impl_->key);
        const EC_POINT* point = EC_KEY_get0_public_key(impl_->key);

        point_conversion_form_t form = compressed ? POINT_CONVERSION_COMPRESSED : POINT_CONVERSION_UNCOMPRESSED;
        size_t size = EC_POINT_point2oct(group, point, form, nullptr, 0, nullptr);

        io::ByteVector result(size);
        EC_POINT_point2oct(group, point, form, result.Data(), size, nullptr);

        return result;
    }

    std::string ECPoint::ToHex(bool compressed) const
    {
        return ToBytes(compressed).ToHexString();
    }

    bool ECPoint::operator==(const ECPoint& other) const
    {
        if (impl_->curveName != other.impl_->curveName)
            return false;

        const EC_GROUP* group = EC_KEY_get0_group(impl_->key);
        const EC_POINT* point1 = EC_KEY_get0_public_key(impl_->key);
        const EC_POINT* point2 = EC_KEY_get0_public_key(other.impl_->key);

        return EC_POINT_cmp(group, point1, point2, nullptr) == 0;
    }

    bool ECPoint::operator!=(const ECPoint& other) const
    {
        return !(*this == other);
    }

    std::string ECPoint::GetCurveName() const
    {
        return impl_->curveName;
    }

    bool ECPoint::IsInfinity() const
    {
        const EC_GROUP* group = EC_KEY_get0_group(impl_->key);
        const EC_POINT* point = EC_KEY_get0_public_key(impl_->key);

        return EC_POINT_is_at_infinity(group, point) == 1;
    }

    std::shared_ptr<ECCurve> ECCurve::GetCurve(const std::string& name)
    {
        static std::unordered_map<std::string, std::shared_ptr<ECCurve>> curves;

        if (curves.empty())
        {
            curves["secp256r1"] = std::make_shared<Secp256r1>();
            curves["secp256k1"] = std::make_shared<Secp256k1>();
        }

        auto it = curves.find(name);
        if (it == curves.end())
            throw std::invalid_argument("Invalid curve name");

        return it->second;
    }

    Secp256r1::Secp256r1()
    {
    }

    std::string Secp256r1::GetName() const
    {
        return "secp256r1";
    }

    ECPoint Secp256r1::GenerateKeyPair(const io::ByteSpan& privateKey) const
    {
        if (privateKey.Size() != GetPrivateKeySize())
            throw std::invalid_argument("Invalid private key size");

        EC_KEY* key = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
        if (!key)
            throw std::runtime_error("Failed to create EC_KEY");

        BIGNUM* bn = BN_bin2bn(privateKey.Data(), static_cast<int>(privateKey.Size()), nullptr);
        if (!bn)
        {
            EC_KEY_free(key);
            throw std::runtime_error("Failed to convert private key to BIGNUM");
        }

        if (!EC_KEY_set_private_key(key, bn))
        {
            BN_free(bn);
            EC_KEY_free(key);
            throw std::runtime_error("Failed to set private key");
        }

        const EC_GROUP* group = EC_KEY_get0_group(key);
        EC_POINT* pub = EC_POINT_new(group);

        if (!EC_POINT_mul(group, pub, bn, nullptr, nullptr, nullptr))
        {
            EC_POINT_free(pub);
            BN_free(bn);
            EC_KEY_free(key);
            throw std::runtime_error("Failed to compute public key");
        }

        if (!EC_KEY_set_public_key(key, pub))
        {
            EC_POINT_free(pub);
            BN_free(bn);
            EC_KEY_free(key);
            throw std::runtime_error("Failed to set public key");
        }

        EC_POINT_free(pub);
        BN_free(bn);

        return ECPoint(std::make_unique<ECPoint::Impl>(key, GetName()));
    }

    io::ByteVector Secp256r1::Sign(const io::ByteSpan& message, const io::ByteSpan& privateKey) const
    {
        if (privateKey.Size() != GetPrivateKeySize())
            throw std::invalid_argument("Invalid private key size");

        // Hash the message with SHA-256
        io::UInt256 hash = Hash::Sha256(message);

        EC_KEY* key = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
        if (!key)
            throw std::runtime_error("Failed to create EC_KEY");

        BIGNUM* bn = BN_bin2bn(privateKey.Data(), static_cast<int>(privateKey.Size()), nullptr);
        if (!bn)
        {
            EC_KEY_free(key);
            throw std::runtime_error("Failed to convert private key to BIGNUM");
        }

        if (!EC_KEY_set_private_key(key, bn))
        {
            BN_free(bn);
            EC_KEY_free(key);
            throw std::runtime_error("Failed to set private key");
        }

        // Generate public key
        const EC_GROUP* group = EC_KEY_get0_group(key);
        EC_POINT* pub = EC_POINT_new(group);

        if (!EC_POINT_mul(group, pub, bn, nullptr, nullptr, nullptr))
        {
            EC_POINT_free(pub);
            BN_free(bn);
            EC_KEY_free(key);
            throw std::runtime_error("Failed to compute public key");
        }

        if (!EC_KEY_set_public_key(key, pub))
        {
            EC_POINT_free(pub);
            BN_free(bn);
            EC_KEY_free(key);
            throw std::runtime_error("Failed to set public key");
        }

        EC_POINT_free(pub);
        BN_free(bn);

        // Sign the hash
        ECDSA_SIG* sig = ECDSA_do_sign(hash.Data(), static_cast<int>(io::UInt256::Size), key);
        if (!sig)
        {
            EC_KEY_free(key);
            throw std::runtime_error("Failed to sign message");
        }

        // Convert signature to DER format
        unsigned char* der = nullptr;
        int derLen = i2d_ECDSA_SIG(sig, &der);
        if (derLen <= 0)
        {
            ECDSA_SIG_free(sig);
            EC_KEY_free(key);
            throw std::runtime_error("Failed to convert signature to DER format");
        }

        io::ByteVector result(io::ByteSpan(der, derLen));

        OPENSSL_free(der);
        ECDSA_SIG_free(sig);
        EC_KEY_free(key);

        return result;
    }

    bool Secp256r1::Verify(const io::ByteSpan& message, const io::ByteSpan& signature, const ECPoint& publicKey) const
    {
        if (publicKey.GetCurveName() != GetName())
            throw std::invalid_argument("Invalid curve for public key");

        // Hash the message with SHA-256
        io::UInt256 hash = Hash::Sha256(message);

        // Parse the signature
        const unsigned char* p = signature.Data();
        ECDSA_SIG* sig = d2i_ECDSA_SIG(nullptr, &p, static_cast<long>(signature.Size()));
        if (!sig)
            return false;

        // Verify the signature
        int result = ECDSA_do_verify(hash.Data(), static_cast<int>(io::UInt256::Size), sig, publicKey.impl_->key);

        ECDSA_SIG_free(sig);

        return result == 1;
    }

    size_t Secp256r1::GetPrivateKeySize() const
    {
        return 32;
    }

    size_t Secp256r1::GetSignatureSize() const
    {
        return 64;
    }

    size_t Secp256r1::GetPublicKeySize() const
    {
        return 65;
    }

    size_t Secp256r1::GetCompressedPublicKeySize() const
    {
        return 33;
    }

    Secp256k1::Secp256k1()
    {
    }

    std::string Secp256k1::GetName() const
    {
        return "secp256k1";
    }

    ECPoint Secp256k1::GenerateKeyPair(const io::ByteSpan& privateKey) const
    {
        if (privateKey.Size() != GetPrivateKeySize())
            throw std::invalid_argument("Invalid private key size");

        EC_KEY* key = EC_KEY_new_by_curve_name(NID_secp256k1);
        if (!key)
            throw std::runtime_error("Failed to create EC_KEY");

        BIGNUM* bn = BN_bin2bn(privateKey.Data(), static_cast<int>(privateKey.Size()), nullptr);
        if (!bn)
        {
            EC_KEY_free(key);
            throw std::runtime_error("Failed to convert private key to BIGNUM");
        }

        if (!EC_KEY_set_private_key(key, bn))
        {
            BN_free(bn);
            EC_KEY_free(key);
            throw std::runtime_error("Failed to set private key");
        }

        const EC_GROUP* group = EC_KEY_get0_group(key);
        EC_POINT* pub = EC_POINT_new(group);

        if (!EC_POINT_mul(group, pub, bn, nullptr, nullptr, nullptr))
        {
            EC_POINT_free(pub);
            BN_free(bn);
            EC_KEY_free(key);
            throw std::runtime_error("Failed to compute public key");
        }

        if (!EC_KEY_set_public_key(key, pub))
        {
            EC_POINT_free(pub);
            BN_free(bn);
            EC_KEY_free(key);
            throw std::runtime_error("Failed to set public key");
        }

        EC_POINT_free(pub);
        BN_free(bn);

        return ECPoint(std::make_unique<ECPoint::Impl>(key, GetName()));
    }

    io::ByteVector Secp256k1::Sign(const io::ByteSpan& message, const io::ByteSpan& privateKey) const
    {
        if (privateKey.Size() != GetPrivateKeySize())
            throw std::invalid_argument("Invalid private key size");

        // Hash the message with Keccak-256
        io::UInt256 hash = Hash::Keccak256(message);

        EC_KEY* key = EC_KEY_new_by_curve_name(NID_secp256k1);
        if (!key)
            throw std::runtime_error("Failed to create EC_KEY");

        BIGNUM* bn = BN_bin2bn(privateKey.Data(), static_cast<int>(privateKey.Size()), nullptr);
        if (!bn)
        {
            EC_KEY_free(key);
            throw std::runtime_error("Failed to convert private key to BIGNUM");
        }

        if (!EC_KEY_set_private_key(key, bn))
        {
            BN_free(bn);
            EC_KEY_free(key);
            throw std::runtime_error("Failed to set private key");
        }

        // Generate public key
        const EC_GROUP* group = EC_KEY_get0_group(key);
        EC_POINT* pub = EC_POINT_new(group);

        if (!EC_POINT_mul(group, pub, bn, nullptr, nullptr, nullptr))
        {
            EC_POINT_free(pub);
            BN_free(bn);
            EC_KEY_free(key);
            throw std::runtime_error("Failed to compute public key");
        }

        if (!EC_KEY_set_public_key(key, pub))
        {
            EC_POINT_free(pub);
            BN_free(bn);
            EC_KEY_free(key);
            throw std::runtime_error("Failed to set public key");
        }

        EC_POINT_free(pub);
        BN_free(bn);

        // Sign the hash
        ECDSA_SIG* sig = ECDSA_do_sign(hash.Data(), static_cast<int>(io::UInt256::Size), key);
        if (!sig)
        {
            EC_KEY_free(key);
            throw std::runtime_error("Failed to sign message");
        }

        // Convert signature to DER format
        unsigned char* der = nullptr;
        int derLen = i2d_ECDSA_SIG(sig, &der);
        if (derLen <= 0)
        {
            ECDSA_SIG_free(sig);
            EC_KEY_free(key);
            throw std::runtime_error("Failed to convert signature to DER format");
        }

        io::ByteVector result(io::ByteSpan(der, derLen));

        OPENSSL_free(der);
        ECDSA_SIG_free(sig);
        EC_KEY_free(key);

        return result;
    }

    bool Secp256k1::Verify(const io::ByteSpan& message, const io::ByteSpan& signature, const ECPoint& publicKey) const
    {
        if (publicKey.GetCurveName() != GetName())
            throw std::invalid_argument("Invalid curve for public key");

        // Hash the message with Keccak-256
        io::UInt256 hash = Hash::Keccak256(message);

        // Parse the signature
        const unsigned char* p = signature.Data();
        ECDSA_SIG* sig = d2i_ECDSA_SIG(nullptr, &p, static_cast<long>(signature.Size()));
        if (!sig)
            return false;

        // Verify the signature
        int result = ECDSA_do_verify(hash.Data(), static_cast<int>(io::UInt256::Size), sig, publicKey.impl_->key);

        ECDSA_SIG_free(sig);

        return result == 1;
    }

    size_t Secp256k1::GetPrivateKeySize() const
    {
        return 32;
    }

    size_t Secp256k1::GetSignatureSize() const
    {
        return 64;
    }

    size_t Secp256k1::GetPublicKeySize() const
    {
        return 65;
    }

    size_t Secp256k1::GetCompressedPublicKeySize() const
    {
        return 33;
    }
}
