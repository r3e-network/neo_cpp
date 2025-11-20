/**
 * @file neo_node.cpp
 * @brief Primary Neo C++ node entrypoint using the shared launcher helper.
 */

#include <neo/node/node_launcher.h>

int main(int argc, char* argv[])
{
    neo::node::app::NodeAppConfig config;
    config.appName = "Neo C++ Node";
    config.defaultConfigPath = "config.json";
    config.defaultDataPathOverride.clear();
    config.allowNetworkPreset = true;
    config.binaryName = "neo_node";

    return neo::node::app::RunNodeApp(argc, argv, config);
}

