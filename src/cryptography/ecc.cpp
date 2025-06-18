#include <neo/cryptography/ecc.h>
#include <neo/cryptography/ecc/ecpoint.h>
#include <neo/cryptography/hash.h>
#include <neo/io/byte_vector.h>

// Suppress OpenSSL deprecation warnings for compatibility
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4996) // OpenSSL deprecation warnings
#endif

#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>
#include <openssl/bn.h>
#include <openssl/err.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <stdexcept>
#include <unordered_map>

namespace neo::cryptography
{
    // This file provides ECC utility functions
    // The main ECC implementations are in the ecc subdirectory
}
