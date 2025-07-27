#include <neo/plugins/plugin_base.h>

namespace neo::plugins
{
PluginBase::PluginBase(const std::string& name, const std::string& description, const std::string& version,
                       const std::string& author)
    : name_(name), description_(description), version_(version), author_(author), running_(false)
{
}

std::string PluginBase::GetName() const
{
    return name_;
}

std::string PluginBase::GetDescription() const
{
    return description_;
}

std::string PluginBase::GetVersion() const
{
    return version_;
}

std::string PluginBase::GetAuthor() const
{
    return author_;
}

bool PluginBase::Initialize(std::shared_ptr<node::NeoSystem> neoSystem,
                            const std::unordered_map<std::string, std::string>& settings)
{
    neoSystem_ = neoSystem;

    return OnInitialize(settings);
}

bool PluginBase::Start()
{
    if (running_)
        return true;

    if (OnStart())
    {
        running_ = true;
        return true;
    }

    return false;
}

bool PluginBase::Stop()
{
    if (!running_)
        return true;

    if (OnStop())
    {
        running_ = false;
        return true;
    }

    return false;
}

bool PluginBase::IsRunning() const
{
    return running_;
}

bool PluginBase::OnInitialize(const std::unordered_map<std::string, std::string>& settings)
{
    return true;
}

bool PluginBase::OnStart()
{
    return true;
}

bool PluginBase::OnStop()
{
    return true;
}

std::shared_ptr<node::NeoSystem> PluginBase::GetNeoSystem() const
{
    return neoSystem_;
}

std::shared_ptr<rpc::RPCServer> PluginBase::GetRPCServer() const
{
    return rpcServer_;
}
}  // namespace neo::plugins
