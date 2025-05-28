#include <neo/protocol_settings.h>

namespace neo
{
    ProtocolSettings::ProtocolSettings()
        : maxTraceableBlocks_(2102400), network_(0x334F454E) // Default values
    {
    }

    uint32_t ProtocolSettings::GetMaxTraceableBlocks() const
    {
        return maxTraceableBlocks_;
    }

    void ProtocolSettings::SetMaxTraceableBlocks(uint32_t maxTraceableBlocks)
    {
        maxTraceableBlocks_ = maxTraceableBlocks;
    }

    uint32_t ProtocolSettings::GetNetwork() const
    {
        return network_;
    }

    void ProtocolSettings::SetNetwork(uint32_t network)
    {
        network_ = network;
    }
}
