/**
 * @file main.cpp
 * @brief Legacy node entrypoint kept for compatibility with downstream build scripts.
 *
 * This now delegates to the shared node launcher to stay consistent with the
 * primary neo_node application.
 */

#include <neo/node/node_launcher.h>

int main(int argc, char* argv[])
{
    neo::node::app::NodeAppConfig config;
    config.appName = "Neo C++ Node (compat)";
    config.defaultConfigPath = "config.json";
    config.defaultDataPathOverride.clear();
    config.allowNetworkPreset = true;
    config.binaryName = "neo_node_exec";

    return neo::node::app::RunNodeApp(argc, argv, config);
}
