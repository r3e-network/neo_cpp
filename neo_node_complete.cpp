/**
 * @file neo_node_complete.cpp
 * @brief Console harness for the production Neo C++ node.
 */

#include <neo/node/node_launcher.h>

#include <iostream>
#include <memory>

namespace
{
void PrintBanner()
{
    std::cout << "============================================\n";
    std::cout << "        Neo N3 C++ Full Node Console        \n";
    std::cout << "============================================\n";
    std::cout << "Build: " << __DATE__ << " " << __TIME__ << "\n";
    std::cout << "Parity target: Neo C# Node 3.7.x\n";
    std::cout << "============================================\n\n";
}

void PrintCompatibilitySummary(const std::shared_ptr<neo::node::NeoNode>& node)
{
    if (!node) return;

    std::cout << "Runtime services mapped to C# components:\n";
    std::cout << "  - Ledger engine     : neo::ledger::Blockchain\n";
    std::cout << "  - Transaction pool  : neo::ledger::MemoryPool\n";
    std::cout << "  - Networking stack  : neo::network::p2p::LocalNode + BlockSyncManager\n";
    std::cout << "  - RPC surface       : neo::rpc::RpcServer (JSON-RPC 2.0)\n";
    std::cout << "  - VM / contracts    : neo::vm + neo::smartcontract::native\n";
    const auto consensus = node->GetConsensusService();
    std::cout << "  - dBFT consensus    : " << (consensus ? "available" : "disabled");
    if (consensus)
    {
        std::cout << " (validators=" << consensus->GetValidators().size()
                  << ", autostart=" << (node->IsConsensusAutoStartEnabled() ? "true" : "false") << ")";
    }
    std::cout << "\n\n";
}
}  // namespace

int main(int argc, char* argv[])
{
    PrintBanner();

    neo::node::app::NodeAppConfig config;
    config.appName = "Neo C++ Full Node";
    config.defaultConfigPath = "config.json";
    config.defaultDataPathOverride = "./data";
    config.allowNetworkPreset = true;
    config.binaryName = "neo_node_complete";

    return neo::node::app::RunNodeApp(
        argc,
        argv,
        config,
        [](const std::shared_ptr<neo::node::NeoNode>& node) { PrintCompatibilitySummary(node); });
}
