/**
 * @file mini_neo_node.cpp
 * @brief Minimal developer launcher using the shared node harness.
 *
 * This entry point is intended for quick local smoke-tests with a lightweight
 * configuration (privnet by default). For production or public networks use
 * neo_node or neo_testnet_node instead.
 */

#include <neo/node/node_launcher.h>

int main(int argc, char* argv[])
{
    neo::node::app::NodeAppConfig config;
    config.appName = "Neo C++ Mini Node";
    config.defaultConfigPath = "config/privnet.json";
    config.defaultDataPathOverride = "./data/privnet-mini";
    config.allowNetworkPreset = true;
    config.binaryName = "mini_neo_node";

    return neo::node::app::RunNodeApp(argc, argv, config);
}
