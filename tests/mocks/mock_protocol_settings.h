#pragma once

#include <gmock/gmock.h>
#include "neo/protocol_settings.h"

namespace neo::tests {

class MockProtocolSettings : public ProtocolSettings {
public:
    MOCK_METHOD(uint32_t, GetNetwork, (), (const, override));
    MOCK_METHOD(uint32_t, GetMagic, (), (const, override));
    MOCK_METHOD(int, GetMaxConnections, (), (const, override));
    MOCK_METHOD(int, GetMaxPeers, (), (const, override));
    MOCK_METHOD(int, GetValidatorsCount, (), (const, override));
    MOCK_METHOD(int, GetMaxTransactionsPerBlock, (), (const, override));
    MOCK_METHOD(size_t, GetMaxBlockSize, (), (const, override));
    MOCK_METHOD(size_t, GetMaxTransactionSize, (), (const, override));
    MOCK_METHOD(uint64_t, GetFeePerByte, (), (const, override));
    MOCK_METHOD(uint32_t, GetMaxValidUntilBlockIncrement, (), (const, override));
    MOCK_METHOD(uint32_t, GetMillisecondsPerBlock, (), (const, override));
    MOCK_METHOD(const std::vector<cryptography::ECPoint>&, GetStandbyCommittee, (), (const, override));
};

} // namespace neo::tests