/**
 * @file neo_testnet_production.cpp
 * @brief Production-style launcher for the Neo testnet using the production presets.
 */

#include <neo/node/node_launcher.h>

int main(int argc, char* argv[])
{
    neo::node::app::NodeAppConfig config;
    config.appName = "Neo C++ Testnet (Production)";
    config.defaultConfigPath = "config/testnet_production.json";
    config.defaultDataPathOverride = "./data/testnet-production";
    config.allowNetworkPreset = true;
    config.binaryName = "neo_testnet_production";

    return neo::node::app::RunNodeApp(argc, argv, config);
}
