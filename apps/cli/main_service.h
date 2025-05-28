#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>

namespace neo
{
    class NeoSystem;
    class Wallet;
    
    namespace cli
    {
        class CommandHandler;
        class TypeConverter;
        class CommandLineOptions;
        
        class MainService
        {
        public:
            MainService();
            ~MainService();
            
            void Run(const std::vector<std::string>& args);
            void Start(const CommandLineOptions& options);
            void Stop();
            
            void RegisterCommand(const std::string& name, const CommandHandler& handler, const std::string& category = "");
            void RegisterCommandHandler(const std::string& typeName, const TypeConverter& converter);
            
            NeoSystem* GetNeoSystem() const;
            Wallet* GetCurrentWallet() const;
            
        private:
            NeoSystem* neoSystem_;
            Wallet* currentWallet_;
            std::unordered_map<std::string, CommandHandler> commands_;
            std::unordered_map<std::string, std::unordered_map<std::string, CommandHandler>> commandsByCategory_;
            std::unordered_map<std::string, TypeConverter> typeConverters_;
            
            void OnStartWithCommandLine(const std::vector<std::string>& args);
            void OnCommand(const std::string& command);
            void OnHelp(const std::string& category = "");
            void OnExit();
            
            // Blockchain Commands
            void OnShowBlock(const std::string& indexOrHash);
            void OnShowHeader(const std::string& indexOrHash);
            void OnShowTransaction(const UInt256& hash);
            void OnShowMerkle(const UInt256& hash);
            
            // Node Commands
            void OnShowState();
            void OnShowPool();
            void OnRelay(const io::ByteVector& data);
            
            // Wallet Commands
            void OnCreateWallet(const std::string& path, const std::string& password);
            void OnOpenWallet(const std::string& path, const std::string& password);
            void OnCloseWallet();
            void OnListAddress();
            void OnListAsset();
            void OnShowUtxo(const UInt160& scriptHash = UInt160());
            
            // Contract Commands
            void OnDeploy(const std::string& filePath, const std::string& manifestPath = "", const json::JObject& data = json::JObject());
            void OnInvoke(const UInt160& scriptHash, const std::string& operation, const std::vector<json::JObject>& args = {}, const std::vector<UInt160>& signers = {});
            
            // NEP-17 Commands
            void OnTransfer(const UInt160& tokenHash, const UInt160& to, const Fixed8& amount, const UInt160& from = UInt160(), const std::string& data = "", const std::vector<UInt160>& signers = {});
            void OnBalanceOf(const UInt160& tokenHash, const UInt160& address);
            
            // Network Commands
            void OnShowPeers();
            void OnConnect(const std::string& address, uint16_t port);
            
            // Plugin Commands
            void OnInstallPlugin(const std::string& pluginName);
            void OnUninstallPlugin(const std::string& pluginName);
            void OnListPlugins();
        };
    }
}
