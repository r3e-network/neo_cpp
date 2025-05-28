#pragma once

#include <cstdint>

namespace neo
{
    /**
     * @brief Represents the hardforks in the NEO system.
     * This enum matches the C# Hardfork enum.
     */
    enum class Hardfork : uint8_t
    {
        HF_Aspidochelone,
        HF_Basilisk,
        HF_Cockatrice,
        HF_Domovoi,
        HF_Echidna
    };
}
