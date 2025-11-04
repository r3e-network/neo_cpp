/**
 * @file plugin_registration.h
 * @brief Plugin registration stubs for the C++ node.
 */

#pragma once

#include <neo/plugins/plugin_manager.h>

namespace neo::plugins
{
/**
 * @brief Registers the default plugin factories with the singleton manager.
 *
 * This mirrors the C# bootstrap step, ensuring core plugins (like application
 * logs) are discoverable even when the embedding host does not explicitly
 * wire them.
 */
void RegisterDefaultPluginFactories();
}  // namespace neo::plugins
