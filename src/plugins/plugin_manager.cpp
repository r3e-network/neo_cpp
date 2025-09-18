/**
 * @file plugin_manager.cpp
 * @brief Management components
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/plugins/plugin.h>
#include <neo/plugins/plugin_base.h>
#include <neo/plugins/plugin_manager.h>
#include <neo/rpc/rpc_server.h>

#include <iostream>

namespace neo::plugins
{
PluginManager& PluginManager::GetInstance()
{
    static PluginManager instance;
    return instance;
}

PluginManager::PluginManager() = default;

void PluginManager::RegisterPluginFactory(std::shared_ptr<PluginFactory> factory) { factories_.push_back(factory); }

const std::vector<std::shared_ptr<PluginFactory>>& PluginManager::GetPluginFactories() const { return factories_; }

const std::vector<std::shared_ptr<Plugin>>& PluginManager::GetPlugins() const { return plugins_; }

std::shared_ptr<Plugin> PluginManager::GetPlugin(const std::string& name) const
{
    for (const auto& plugin : plugins_)
    {
        if (plugin->GetName() == name) return plugin;
    }

    return nullptr;
}

bool PluginManager::LoadPlugins(std::shared_ptr<node::NeoSystem> neoSystem,
                                const std::unordered_map<std::string, std::string>& settings,
                                std::shared_ptr<rpc::RpcServer> rpcServer)
{
    bool result = true;

    // Create plugins
    for (const auto& factory : factories_)
    {
        try
        {
            auto plugin = factory->CreatePlugin();

            // Initialize plugin
            if (plugin->Initialize(neoSystem, settings))
            {
                // Inject RPC server when available and plugin supports it
                if (rpcServer)
                {
                    if (auto* base = dynamic_cast<neo::plugins::PluginBase*>(plugin.get()))
                    {
                        base->SetRPCServer(rpcServer);
                    }
                }
                plugins_.push_back(plugin);
                std::cout << "Loaded plugin: " << plugin->GetName() << " v" << plugin->GetVersion() << " by "
                          << plugin->GetAuthor() << std::endl;
            }
            else
            {
                std::cerr << "Failed to initialize plugin: " << plugin->GetName() << std::endl;
                result = false;
            }
        }
        catch (const std::exception& ex)
        {
            std::cerr << "Failed to create plugin: " << ex.what() << std::endl;
            result = false;
        }
    }

    return result;
}

bool PluginManager::StartPlugins()
{
    bool result = true;

    for (const auto& plugin : plugins_)
    {
        try
        {
            if (plugin->Start())
            {
                std::cout << "Started plugin: " << plugin->GetName() << std::endl;
            }
            else
            {
                std::cerr << "Failed to start plugin: " << plugin->GetName() << std::endl;
                result = false;
            }
        }
        catch (const std::exception& ex)
        {
            std::cerr << "Failed to start plugin: " << plugin->GetName() << " - " << ex.what() << std::endl;
            result = false;
        }
    }

    return result;
}

bool PluginManager::StopPlugins()
{
    bool result = true;

    for (const auto& plugin : plugins_)
    {
        try
        {
            if (plugin->Stop())
            {
                std::cout << "Stopped plugin: " << plugin->GetName() << std::endl;
            }
            else
            {
                std::cerr << "Failed to stop plugin: " << plugin->GetName() << std::endl;
                result = false;
            }
        }
        catch (const std::exception& ex)
        {
            std::cerr << "Failed to stop plugin: " << plugin->GetName() << " - " << ex.what() << std::endl;
            result = false;
        }
    }

    return result;
}
}  // namespace neo::plugins
