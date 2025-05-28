#pragma once

#include <neo/io/json_serializable.h>
#include <cstdint>

namespace neo::wallets::nep6
{
    /**
     * @brief Represents the parameters of the SCrypt algorithm.
     */
    class ScryptParameters : public io::JsonSerializable
    {
    public:
        /**
         * @brief Constructs a ScryptParameters with default values.
         */
        ScryptParameters();

        /**
         * @brief Constructs a ScryptParameters with the specified values.
         * @param n The CPU/Memory cost parameter.
         * @param r The block size.
         * @param p The parallelization parameter.
         */
        ScryptParameters(uint32_t n, uint32_t r, uint32_t p);

        /**
         * @brief Gets the CPU/Memory cost parameter.
         * @return The CPU/Memory cost parameter.
         */
        uint32_t GetN() const;

        /**
         * @brief Sets the CPU/Memory cost parameter.
         * @param n The CPU/Memory cost parameter.
         */
        void SetN(uint32_t n);

        /**
         * @brief Gets the block size.
         * @return The block size.
         */
        uint32_t GetR() const;

        /**
         * @brief Sets the block size.
         * @param r The block size.
         */
        void SetR(uint32_t r);

        /**
         * @brief Gets the parallelization parameter.
         * @return The parallelization parameter.
         */
        uint32_t GetP() const;

        /**
         * @brief Sets the parallelization parameter.
         * @param p The parallelization parameter.
         */
        void SetP(uint32_t p);

        /**
         * @brief Gets the default ScryptParameters.
         * @return The default ScryptParameters.
         */
        static ScryptParameters Default();

        /**
         * @brief Serializes the ScryptParameters to a JSON object.
         * @return The JSON object.
         */
        nlohmann::json ToJson() const override;

        /**
         * @brief Deserializes the ScryptParameters from a JSON object.
         * @param json The JSON object.
         */
        void FromJson(const nlohmann::json& json) override;

    private:
        uint32_t n_;
        uint32_t r_;
        uint32_t p_;
    };
}
