/**
 * @file neo_testnet_node.cpp
 * @brief Thin wrapper to start a Neo C++ node on the public testnet.
 */

#include <neo/node/node_launcher.h>

int main(int argc, char* argv[])
{
    neo::node::app::NodeAppConfig config;
    config.appName = "Neo C++ Testnet Node";
    config.defaultConfigPath = "config/testnet.config.json";
    config.defaultDataPathOverride = "./data/testnet";
    config.allowNetworkPreset = true;
    config.binaryName = "neo_testnet_node";

    return neo::node::app::RunNodeApp(argc, argv, config);
}
