#pragma once

namespace neo::constants {

// Time constants
constexpr int SECONDS_PER_MINUTE = 60;
constexpr int SECONDS_PER_HOUR = 3600;
constexpr int SECONDS_PER_DAY = 86400;
constexpr int MILLISECONDS_PER_SECOND = 1000;

// Size constants
constexpr size_t KB = 1024;
constexpr size_t MB = 1024 * KB;
constexpr size_t GB = 1024 * MB;

// Network constants
constexpr size_t MAX_MESSAGE_SIZE = 2 * MB;
constexpr size_t MAX_BLOCK_SIZE = 1 * MB;
constexpr size_t MAX_TRANSACTION_SIZE = 100 * KB;
constexpr size_t MAX_SCRIPT_SIZE = 65536;

// Consensus constants
constexpr size_t MIN_VALIDATORS = 4;
constexpr size_t MAX_VALIDATORS = 1024;
constexpr int BLOCK_TIME_SECONDS = 15;

// Cryptography constants
constexpr size_t HASH_SIZE = 32;
constexpr size_t SIGNATURE_SIZE = 64;
constexpr size_t PUBLIC_KEY_SIZE = 33;
constexpr size_t PRIVATE_KEY_SIZE = 32;

// Cache constants
constexpr size_t DEFAULT_CACHE_SIZE = 1000;
constexpr size_t MAX_CACHE_SIZE = 10000;

// Performance constants
constexpr size_t DEFAULT_THREAD_POOL_SIZE = 8;
constexpr size_t MAX_CONCURRENT_REQUESTS = 1000;

} // namespace neo::constants
