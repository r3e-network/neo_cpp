#include <neo/cryptography/ecrecover.h>
#include <neo/cryptography/ecc/ecpoint.h>
#include <neo/cryptography/hash.h>
#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/evp.h>
#include <neo/core/logging.h>
#include <memory>
#include <vector>

namespace neo::cryptography
{

// RAII wrapper for BIGNUM
struct BNDeleter
{
    void operator()(BIGNUM* bn) const { if (bn) BN_free(bn); }
};
using BNPtr = std::unique_ptr<BIGNUM, BNDeleter>;

// RAII wrapper for EC_POINT
struct ECPointDeleter
{
    void operator()(EC_POINT* point) const { if (point) EC_POINT_free(point); }
};
using ECPointPtr = std::unique_ptr<EC_POINT, ECPointDeleter>;

// RAII wrapper for EC_KEY
struct ECKeyDeleter
{
    void operator()(EC_KEY* key) const { if (key) EC_KEY_free(key); }
};
using ECKeyPtr = std::unique_ptr<EC_KEY, ECKeyDeleter>;

std::optional<ecc::ECPoint> ECRecover(const io::ByteSpan& hash, const io::ByteSpan& signature, uint8_t recovery_id)
{
    if (hash.Size() != 32 || signature.Size() != 64 || recovery_id > 3)
    {
        return std::nullopt;
    }

    // Create secp256k1 curve (Ethereum uses secp256k1, not secp256r1)
    ECKeyPtr ec_key(EC_KEY_new_by_curve_name(NID_secp256k1));
    if (!ec_key)
    {
        LOG_ERROR("Failed to create EC_KEY for secp256k1");
        return std::nullopt;
    }

    const EC_GROUP* group = EC_KEY_get0_group(ec_key.get());
    if (!group)
    {
        LOG_error("Failed to get EC_GROUP");
        return std::nullopt;
    }

    // Extract r and s from signature
    BNPtr r(BN_bin2bn(signature.Data(), 32, nullptr));
    BNPtr s(BN_bin2bn(signature.Data() + 32, 32, nullptr));
    BNPtr order(BN_new());
    BNPtr x(BN_new());

    if (!r || !s || !order || !x)
    {
        LOG_error("Failed to allocate BIGNUMs");
        return std::nullopt;
    }

    // Get curve order
    if (!EC_GROUP_get_order(group, order.get(), nullptr))
    {
        LOG_error("Failed to get curve order");
        return std::nullopt;
    }

    // Calculate x coordinate: x = r + (recovery_id / 2) * order
    if (recovery_id >= 2)
    {
        if (!BN_add(x.get(), r.get(), order.get()))
        {
            LOG_error("Failed to add order to r");
            return std::nullopt;
        }
    }
    else
    {
        if (!BN_copy(x.get(), r.get()))
        {
            LOG_error("Failed to copy r to x");
            return std::nullopt;
        }
    }

    // Recover the point
    ECPointPtr recovered_point(EC_POINT_new(group));
    if (!recovered_point)
    {
        LOG_error("Failed to create recovered_point");
        return std::nullopt;
    }

    // Set x coordinate and determine y
    if (!EC_POINT_set_compressed_coordinates_GFp(group, recovered_point.get(), x.get(), recovery_id & 1, nullptr))
    {
        LOG_error("Failed to set compressed coordinates");
        return std::nullopt;
    }

    // Verify the point is valid
    if (!EC_POINT_is_on_curve(group, recovered_point.get(), nullptr))
    {
        LOG_error("Recovered point is not on curve");
        return std::nullopt;
    }

    // Calculate the public key using ECDSA recovery formula
    // Q = (r^-1) * (s * R - e * G)
    BNPtr r_inv(BN_new());
    BNPtr e(BN_bin2bn(hash.Data(), hash.Size(), nullptr));
    ECPointPtr point1(EC_POINT_new(group));
    ECPointPtr point2(EC_POINT_new(group));
    ECPointPtr result(EC_POINT_new(group));

    if (!r_inv || !e || !point1 || !point2 || !result)
    {
        LOG_error("Failed to allocate recovery objects");
        return std::nullopt;
    }

    // Calculate r^-1 mod order
    BN_CTX* ctx = BN_CTX_new();
    if (!ctx)
    {
        LOG_error("Failed to create BN_CTX");
        return std::nullopt;
    }

    bool success = false;
    if (BN_mod_inverse(r_inv.get(), r.get(), order.get(), ctx))
    {
        // Calculate s * R
        if (EC_POINT_mul(group, point1.get(), nullptr, recovered_point.get(), s.get(), ctx))
        {
            // Calculate e * G
            if (EC_POINT_mul(group, point2.get(), e.get(), nullptr, nullptr, ctx))
            {
                // Calculate s * R - e * G
                if (EC_POINT_invert(group, point2.get(), ctx))
                {
                    if (EC_POINT_add(group, result.get(), point1.get(), point2.get(), ctx))
                    {
                        // Calculate r^-1 * (s * R - e * G)
                        if (EC_POINT_mul(group, result.get(), nullptr, result.get(), r_inv.get(), ctx))
                        {
                            success = true;
                        }
                    }
                }
            }
        }
    }

    BN_CTX_free(ctx);

    if (!success)
    {
        LOG_error("Failed during recovery calculation");
        return std::nullopt;
    }

    // Convert to compressed format
    size_t point_len = EC_POINT_point2oct(group, result.get(), POINT_CONVERSION_COMPRESSED, nullptr, 0, nullptr);
    if (point_len == 0)
    {
        LOG_error("Failed to get point length");
        return std::nullopt;
    }

    std::vector<uint8_t> point_data(point_len);
    if (EC_POINT_point2oct(group, result.get(), POINT_CONVERSION_COMPRESSED, 
                          point_data.data(), point_len, nullptr) != point_len)
    {
        LOG_error("Failed to convert point to octets");
        return std::nullopt;
    }

    // Create ECPoint from the recovered data
    try
    {
        // Use secp256k1 curve name for Ethereum compatibility
        auto recovered_ecpoint = ecc::ECPoint::FromBytes(io::ByteSpan(point_data.data(), point_len), "secp256k1");
        return recovered_ecpoint;
    }
    catch (const std::exception& e)
    {
        LOG_error("Failed to create ECPoint from recovered data: {}", e.what());
        return std::nullopt;
    }
}

} // namespace neo::cryptography