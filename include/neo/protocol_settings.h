#pragma once

#include <cstdint>

namespace neo
{
    /**
     * @brief Represents the protocol settings of the NEO system.
     * This is a temporary implementation to get the build working.
     */
    class ProtocolSettings
    {
    public:
        /**
         * @brief Constructs default ProtocolSettings.
         */
        ProtocolSettings();

        /**
         * @brief Gets the maximum number of traceable blocks.
         * @return The maximum number of traceable blocks.
         */
        uint32_t GetMaxTraceableBlocks() const;

        /**
         * @brief Sets the maximum number of traceable blocks.
         * @param maxTraceableBlocks The maximum number of traceable blocks.
         */
        void SetMaxTraceableBlocks(uint32_t maxTraceableBlocks);

        /**
         * @brief Gets the network magic number.
         * @return The network magic number.
         */
        uint32_t GetNetwork() const;

        /**
         * @brief Sets the network magic number.
         * @param network The network magic number.
         */
        void SetNetwork(uint32_t network);

    private:
        uint32_t maxTraceableBlocks_;
        uint32_t network_;
    };
}
