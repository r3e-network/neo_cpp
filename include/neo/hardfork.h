#pragma once
#include <string>

/**
 * @file hardfork.h
 * @brief Hardfork enumeration for Neo protocol upgrades
 *
 * This header defines the hardfork types used in the Neo blockchain
 * to manage protocol upgrades and feature activations.
 */

namespace neo
{
/**
 * @brief Enumeration of Neo protocol hardforks
 *
 * Each hardfork represents a protocol upgrade that activates
 * at a specific block height. The order is important as it
 * determines the chronological sequence of upgrades.
 */
enum class Hardfork
{
    /**
     * @brief Aspidochelone hardfork
     *
     * First major hardfork introducing various protocol improvements.
     */
    HF_Aspidochelone = 0,

    /**
     * @brief Basilisk hardfork
     *
     * Second hardfork with additional protocol enhancements.
     */
    HF_Basilisk = 1,

    /**
     * @brief Cockatrice hardfork
     *
     * Third hardfork with further protocol improvements.
     */
    HF_Cockatrice = 2,

    /**
     * @brief Domovoi hardfork
     *
     * Fourth hardfork introducing new features.
     */
    HF_Domovoi = 3,

    /**
     * @brief Echidna hardfork
     *
     * Fifth hardfork with significant protocol changes including
     * dynamic block time and traceable blocks management.
     */
    HF_Echidna = 4
};

/**
 * @brief Get the total number of defined hardforks
 * @return The count of hardforks
 */
constexpr int GetHardforkCount() { return static_cast<int>(Hardfork::HF_Echidna) + 1; }

/**
 * @brief Convert hardfork enum to string representation
 * @param hardfork The hardfork to convert
 * @return String name of the hardfork
 */
const char* HardforkToString(Hardfork hardfork);

/**
 * @brief Parse string to hardfork enum
 * @param str The string to parse
 * @return The corresponding hardfork enum
 * @throws std::invalid_argument if string is not a valid hardfork name
 */
Hardfork StringToHardfork(const std::string& str);
}  // namespace neo
