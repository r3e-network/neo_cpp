#pragma once

// Redirect to the correct location
#include <neo/wallets/key_pair.h>

// Type alias for compatibility
namespace neo::cryptography
{
    using KeyPair = neo::wallets::KeyPair;
} 