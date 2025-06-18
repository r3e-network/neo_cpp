#pragma once

#include <gmock/gmock.h>
#include "neo/node/neo_system.h"

namespace neo::tests {

class MockNeoSystem : public node::NeoSystem {
public:
    MOCK_METHOD(std::shared_ptr<ProtocolSettings>, GetSettings, (), (const, override));
    MOCK_METHOD(std::shared_ptr<ledger::Blockchain>, GetBlockchain, (), (const, override));
    MOCK_METHOD(std::shared_ptr<ledger::MemoryPool>, GetMemoryPool, (), (const, override));
    MOCK_METHOD(std::shared_ptr<network::P2PServer>, GetP2PServer, (), (const, override));
    MOCK_METHOD(std::shared_ptr<rpc::RpcServer>, GetRpcServer, (), (const, override));
    MOCK_METHOD(void, Start, (), (override));
    MOCK_METHOD(void, Stop, (), (override));
    MOCK_METHOD(bool, IsRunning, (), (const, override));
};

} // namespace neo::tests