#include <neo/extensions/datetime_extensions.h>
#include <neo/extensions/random_extensions.h>

#include <algorithm>
#include <chrono>
#include <limits>
#include <random>
#include <thread>

#ifdef _WIN32
#include <wincrypt.h>
#include <windows.h>
#else
#include <unistd.h>

#include <fstream>
#endif

namespace neo::extensions
{
io::ByteVector RandomExtensions::GenerateRandomBytes(size_t length)
{
    io::ByteVector result(length);

#ifdef _WIN32
    // Use Windows CryptGenRandom for secure random bytes
    HCRYPTPROV hProv;
    if (CryptAcquireContext(&hProv, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
    {
        CryptGenRandom(hProv, static_cast<DWORD>(length), result.Data());
        CryptReleaseContext(hProv, 0);
    }
    else
    {
        // Fallback to std::random_device
        std::random_device rd;
        for (size_t i = 0; i < length; ++i)
        {
            result[i] = static_cast<uint8_t>(rd() & 0xFF);
        }
    }
#else
    // Use /dev/urandom on Unix-like systems
    std::ifstream urandom("/dev/urandom", std::ios::binary);
    if (urandom.good())
    {
        urandom.read(reinterpret_cast<char*>(result.Data()), length);
    }
    else
    {
        // Fallback to std::random_device
        std::random_device rd;
        for (size_t i = 0; i < length; ++i)
        {
            result[i] = static_cast<uint8_t>(rd() & 0xFF);
        }
    }
#endif

    return result;
}

int32_t RandomExtensions::NextInt(int32_t min, int32_t max)
{
    if (min > max) std::swap(min, max);

    auto& rng = GetSecureRNG();
    std::uniform_int_distribution<int32_t> dist(min, max);
    return dist(rng);
}

uint32_t RandomExtensions::NextUInt(uint32_t min, uint32_t max)
{
    if (min > max) std::swap(min, max);

    auto& rng = GetSecureRNG();
    std::uniform_int_distribution<uint32_t> dist(min, max);
    return dist(rng);
}

int64_t RandomExtensions::NextLong(int64_t min, int64_t max)
{
    if (min > max) std::swap(min, max);

    auto& rng = GetSecureRNG();
    std::uniform_int_distribution<int64_t> dist(min, max);
    return dist(rng);
}

uint64_t RandomExtensions::NextULong(uint64_t min, uint64_t max)
{
    if (min > max) std::swap(min, max);

    auto& rng = GetSecureRNG();
    std::uniform_int_distribution<uint64_t> dist(min, max);
    return dist(rng);
}

float RandomExtensions::NextFloat()
{
    auto& rng = GetSecureRNG();
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    return dist(rng);
}

float RandomExtensions::NextFloat(float min, float max)
{
    if (min > max) std::swap(min, max);

    auto& rng = GetSecureRNG();
    std::uniform_real_distribution<float> dist(min, max);
    return dist(rng);
}

double RandomExtensions::NextDouble()
{
    auto& rng = GetSecureRNG();
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    return dist(rng);
}

double RandomExtensions::NextDouble(double min, double max)
{
    if (min > max) std::swap(min, max);

    auto& rng = GetSecureRNG();
    std::uniform_real_distribution<double> dist(min, max);
    return dist(rng);
}

io::UInt160 RandomExtensions::GenerateRandomUInt160()
{
    auto randomBytes = GenerateRandomBytes(20);
    return io::UInt160(randomBytes.AsSpan());
}

io::UInt256 RandomExtensions::GenerateRandomUInt256()
{
    auto randomBytes = GenerateRandomBytes(32);
    return io::UInt256(randomBytes.AsSpan());
}

std::string RandomExtensions::GenerateRandomString(size_t length)
{
    const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    std::string result;
    result.reserve(length);

    auto& rng = GetSecureRNG();
    std::uniform_int_distribution<size_t> dist(0, chars.length() - 1);

    for (size_t i = 0; i < length; ++i)
    {
        result += chars[dist(rng)];
    }

    return result;
}

std::string RandomExtensions::GenerateRandomHexString(size_t length)
{
    const std::string hexChars = "0123456789ABCDEF";
    std::string result;
    result.reserve(length);

    auto& rng = GetSecureRNG();
    std::uniform_int_distribution<size_t> dist(0, hexChars.length() - 1);

    for (size_t i = 0; i < length; ++i)
    {
        result += hexChars[dist(rng)];
    }

    return result;
}

uint64_t RandomExtensions::GenerateRandomTimestamp(uint64_t baseTime, uint64_t maxVariation)
{
    if (baseTime == 0)
    {
        baseTime = DateTimeExtensions::GetUnixTimestamp();
    }

    auto variation = NextULong(0, maxVariation * 2) - maxVariation;
    return baseTime + variation;
}

std::mt19937& RandomExtensions::GetSecureRNG()
{
    // Thread-local secure random number generator
    thread_local std::mt19937 rng;
    thread_local bool initialized = false;

    if (!initialized)
    {
        SeedSecureRNG(rng);
        initialized = true;
    }

    return rng;
}

void RandomExtensions::SeedSecureRNG(std::mt19937& rng)
{
    // Create a seed sequence using multiple entropy sources
    std::vector<uint32_t> seeds;

    // High-resolution time
    auto now = std::chrono::high_resolution_clock::now();
    auto nanos = now.time_since_epoch().count();
    seeds.push_back(static_cast<uint32_t>(nanos));
    seeds.push_back(static_cast<uint32_t>(nanos >> 32));

    // Thread ID
    auto threadId = std::hash<std::thread::id>{}(std::this_thread::get_id());
    seeds.push_back(static_cast<uint32_t>(threadId));
    seeds.push_back(static_cast<uint32_t>(threadId >> 32));

    // Process ID
#ifdef _WIN32
    seeds.push_back(GetCurrentProcessId());
#else
    seeds.push_back(getpid());
#endif

    // Random device (hardware entropy if available)
    std::random_device rd;
    for (int i = 0; i < 4; ++i)
    {
        seeds.push_back(rd());
    }

    // Secure random bytes as additional entropy
    auto randomBytes = GenerateRandomBytes(16);
    for (size_t i = 0; i < 16; i += 4)
    {
        uint32_t seed = 0;
        for (int j = 0; j < 4 && (i + j) < 16; ++j)
        {
            seed |= (static_cast<uint32_t>(randomBytes[i + j]) << (j * 8));
        }
        seeds.push_back(seed);
    }

    // Create seed sequence and seed the generator
    std::seed_seq seedSeq(seeds.begin(), seeds.end());
    rng.seed(seedSeq);
}
}  // namespace neo::extensions
