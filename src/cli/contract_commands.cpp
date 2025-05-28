#include <neo/cli/command_handler.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/json.h>
#include <iostream>
#include <fstream>
#include <filesystem>

namespace neo::cli
{
    bool CommandHandler::HandleDeploy(const std::vector<std::string>& args)
    {
        if (!wallet_)
        {
            std::cout << "No wallet is open" << std::endl;
            return false;
        }
        
        if (args.size() < 1)
        {
            std::cout << "Usage: deploy <nef-path> [manifest-path]" << std::endl;
            return false;
        }
        
        std::string nefPath = args[0];
        std::string manifestPath;
        
        if (args.size() >= 2)
        {
            manifestPath = args[1];
        }
        else
        {
            // Try to infer manifest path
            std::filesystem::path path(nefPath);
            path.replace_extension(".manifest.json");
            manifestPath = path.string();
        }
        
        try
        {
            // Read NEF file
            std::ifstream nefFile(nefPath, std::ios::binary);
            if (!nefFile)
            {
                std::cout << "Failed to open NEF file: " << nefPath << std::endl;
                return false;
            }
            
            nefFile.seekg(0, std::ios::end);
            size_t nefSize = nefFile.tellg();
            nefFile.seekg(0, std::ios::beg);
            
            std::vector<uint8_t> nefData(nefSize);
            nefFile.read(reinterpret_cast<char*>(nefData.data()), nefSize);
            
            // Read manifest file
            std::ifstream manifestFile(manifestPath);
            if (!manifestFile)
            {
                std::cout << "Failed to open manifest file: " << manifestPath << std::endl;
                return false;
            }
            
            nlohmann::json manifestJson;
            manifestFile >> manifestJson;
            
            // Create NEF
            smartcontract::NefFile nef;
            
            std::istringstream nefStream(std::string(reinterpret_cast<const char*>(nefData.data()), nefData.size()));
            io::BinaryReader nefReader(nefStream);
            nef.Deserialize(nefReader);
            
            // Create manifest
            smartcontract::ContractManifest manifest;
            manifest.FromJson(manifestJson);
            
            // Create deploy transaction
            auto tx = wallet_->CreateDeployTransaction(nef, manifest);
            
            // Sign transaction
            wallet_->SignTransaction(tx);
            
            // Send transaction
            auto result = node_->GetMemoryPool()->AddTransaction(tx);
            
            if (result)
            {
                std::cout << "Contract deployed: " << tx->GetHash().ToString() << std::endl;
                
                // Calculate contract hash
                auto contractHash = smartcontract::ContractState::CalculateHash(tx->GetSender(), nef.GetChecksum(), manifest.GetName());
                std::cout << "Contract hash: " << contractHash.ToString() << std::endl;
                
                return true;
            }
            else
            {
                std::cout << "Failed to deploy contract" << std::endl;
                return false;
            }
        }
        catch (const std::exception& ex)
        {
            std::cout << "Failed to deploy contract: " << ex.what() << std::endl;
            return false;
        }
    }
    
    bool CommandHandler::HandleInvoke(const std::vector<std::string>& args)
    {
        if (!wallet_)
        {
            std::cout << "No wallet is open" << std::endl;
            return false;
        }
        
        if (args.size() < 2)
        {
            std::cout << "Usage: invoke <script-hash> <method> [params...]" << std::endl;
            return false;
        }
        
        std::string scriptHashStr = args[0];
        std::string method = args[1];
        
        try
        {
            // Parse script hash
            io::UInt160 scriptHash;
            scriptHash.FromString(scriptHashStr);
            
            // Parse parameters
            std::vector<std::shared_ptr<smartcontract::vm::StackItem>> params;
            
            for (size_t i = 2; i < args.size(); i++)
            {
                std::string param = args[i];
                
                // Try to parse parameter
                if (param == "true")
                {
                    params.push_back(smartcontract::vm::StackItem::Create(true));
                }
                else if (param == "false")
                {
                    params.push_back(smartcontract::vm::StackItem::Create(false));
                }
                else if (param.find_first_not_of("0123456789") == std::string::npos)
                {
                    // Integer
                    params.push_back(smartcontract::vm::StackItem::Create(std::stoll(param)));
                }
                else if (param.size() >= 2 && param[0] == '"' && param[param.size() - 1] == '"')
                {
                    // String
                    params.push_back(smartcontract::vm::StackItem::Create(param.substr(1, param.size() - 2)));
                }
                else if (param.size() >= 2 && param[0] == '[' && param[param.size() - 1] == ']')
                {
                    // Array
                    std::vector<std::shared_ptr<smartcontract::vm::StackItem>> array;
                    
                    // Parse array elements
                    std::string elements = param.substr(1, param.size() - 2);
                    std::istringstream elementStream(elements);
                    std::string element;
                    
                    while (std::getline(elementStream, element, ','))
                    {
                        // Trim whitespace
                        element.erase(0, element.find_first_not_of(" \t"));
                        element.erase(element.find_last_not_of(" \t") + 1);
                        
                        if (element == "true")
                        {
                            array.push_back(smartcontract::vm::StackItem::Create(true));
                        }
                        else if (element == "false")
                        {
                            array.push_back(smartcontract::vm::StackItem::Create(false));
                        }
                        else if (element.find_first_not_of("0123456789") == std::string::npos)
                        {
                            // Integer
                            array.push_back(smartcontract::vm::StackItem::Create(std::stoll(element)));
                        }
                        else if (element.size() >= 2 && element[0] == '"' && element[element.size() - 1] == '"')
                        {
                            // String
                            array.push_back(smartcontract::vm::StackItem::Create(element.substr(1, element.size() - 2)));
                        }
                        else
                        {
                            std::cout << "Invalid array element: " << element << std::endl;
                            return false;
                        }
                    }
                    
                    params.push_back(smartcontract::vm::StackItem::Create(array));
                }
                else
                {
                    // Try to parse as hex
                    try
                    {
                        auto data = io::ByteVector::FromHex(param);
                        params.push_back(smartcontract::vm::StackItem::Create(data));
                    }
                    catch (const std::exception&)
                    {
                        std::cout << "Invalid parameter: " << param << std::endl;
                        return false;
                    }
                }
            }
            
            // Create invocation transaction
            auto tx = wallet_->CreateInvocationTransaction(scriptHash, method, params);
            
            // Sign transaction
            wallet_->SignTransaction(tx);
            
            // Send transaction
            auto result = node_->GetMemoryPool()->AddTransaction(tx);
            
            if (result)
            {
                std::cout << "Transaction sent: " << tx->GetHash().ToString() << std::endl;
                return true;
            }
            else
            {
                std::cout << "Failed to send transaction" << std::endl;
                return false;
            }
        }
        catch (const std::exception& ex)
        {
            std::cout << "Failed to invoke contract: " << ex.what() << std::endl;
            return false;
        }
    }
}
