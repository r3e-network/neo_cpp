/**
 * @file safe_type_converters.cpp
 * @brief Type conversion utilities
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/cli/safe_type_converters.h>
#include <neo/core/safe_conversions.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>

#include <sstream>

namespace neo::cli
{

void SafeTypeConverters::InitializeDefaultConverters()
{
    // Register string converter
    RegisterConverter<std::string>("string",
                                   [](const std::vector<std::string>& args, bool canConsumeAll) -> std::string
                                   {
                                       if (args.empty())
                                       {
                                           return std::string("");
                                       }

                                       if (canConsumeAll)
                                       {
                                           std::ostringstream result;
                                           for (size_t i = 0; i < args.size(); i++)
                                           {
                                               if (i > 0)
                                               {
                                                   result << " ";
                                               }
                                               result << args[i];
                                           }
                                           return result.str();
                                       }

                                       return args[0];
                                   });

    // Register UInt256 converter
    RegisterConverter<io::UInt256>("UInt256",
                                   [](const std::vector<std::string>& args, bool canConsumeAll) -> io::UInt256
                                   {
                                       (void)canConsumeAll;  // Suppress unused parameter warning
                                       if (args.empty())
                                       {
                                           throw std::runtime_error("Missing argument for UInt256");
                                       }

                                       try
                                       {
                                           return io::UInt256::Parse(args[0]);
                                       }
                                       catch (const std::exception& e)
                                       {
                                           throw std::runtime_error("Invalid UInt256 format: " + std::string(e.what()));
                                       }
                                   });

    // Register UInt160 converter
    RegisterConverter<io::UInt160>("UInt160",
                                   [](const std::vector<std::string>& args, bool canConsumeAll) -> io::UInt160
                                   {
                                       (void)canConsumeAll;  // Suppress unused parameter warning
                                       if (args.empty())
                                       {
                                           throw std::runtime_error("Missing argument for UInt160");
                                       }

                                       try
                                       {
                                           return io::UInt160::Parse(args[0]);
                                       }
                                       catch (const std::exception& e)
                                       {
                                           throw std::runtime_error("Invalid UInt160 format: " + std::string(e.what()));
                                       }
                                   });

    // Register int converter with safe conversion
    RegisterConverter<int>("int",
                           [](const std::vector<std::string>& args, bool canConsumeAll) -> int
                           {
                               (void)canConsumeAll;  // Suppress unused parameter warning
                               if (args.empty())
                               {
                                   throw std::runtime_error("Missing argument for int");
                               }

                               try
                               {
                                   return core::SafeConversions::SafeToInt32(args[0]);
                               }
                               catch (const std::exception& e)
                               {
                                   throw std::runtime_error("Invalid int format: " + std::string(e.what()));
                               }
                           });

    // Register uint32_t converter with safe conversion
    RegisterConverter<uint32_t>("uint32",
                                [](const std::vector<std::string>& args, bool canConsumeAll) -> uint32_t
                                {
                                    (void)canConsumeAll;  // Suppress unused parameter warning
                                    if (args.empty())
                                    {
                                        throw std::runtime_error("Missing argument for uint32");
                                    }

                                    try
                                    {
                                        return core::SafeConversions::SafeToUInt32(args[0]);
                                    }
                                    catch (const std::exception& e)
                                    {
                                        throw std::runtime_error("Invalid uint32 format: " + std::string(e.what()));
                                    }
                                });

    // Register uint16_t converter for ports
    RegisterConverter<uint16_t>("port",
                                [](const std::vector<std::string>& args, bool canConsumeAll) -> uint16_t
                                {
                                    (void)canConsumeAll;  // Suppress unused parameter warning
                                    if (args.empty())
                                    {
                                        throw std::runtime_error("Missing argument for port");
                                    }

                                    try
                                    {
                                        return core::SafeConversions::SafeToPort(args[0]);
                                    }
                                    catch (const std::exception& e)
                                    {
                                        throw std::runtime_error("Invalid port format: " + std::string(e.what()));
                                    }
                                });

    // Register bool converter
    RegisterConverter<bool>("bool",
                            [](const std::vector<std::string>& args, bool canConsumeAll) -> bool
                            {
                                (void)canConsumeAll;  // Suppress unused parameter warning
                                if (args.empty())
                                {
                                    throw std::runtime_error("Missing argument for bool");
                                }

                                std::string str = args[0];
                                std::transform(str.begin(), str.end(), str.begin(), ::tolower);

                                if (str == "true" || str == "1" || str == "yes" || str == "y")
                                {
                                    return true;
                                }
                                else if (str == "false" || str == "0" || str == "no" || str == "n")
                                {
                                    return false;
                                }
                                else
                                {
                                    throw std::runtime_error("Invalid bool format: " + args[0] +
                                                             " (expected: true/false, 1/0, yes/no, y/n)");
                                }
                            });

    // Register double converter with safe conversion
    RegisterConverter<double>("double",
                              [](const std::vector<std::string>& args, bool canConsumeAll) -> double
                              {
                                  (void)canConsumeAll;  // Suppress unused parameter warning
                                  if (args.empty())
                                  {
                                      throw std::runtime_error("Missing argument for double");
                                  }

                                  try
                                  {
                                      return core::SafeConversions::SafeToDouble(args[0]);
                                  }
                                  catch (const std::exception& e)
                                  {
                                      throw std::runtime_error("Invalid double format: " + std::string(e.what()));
                                  }
                              });
}

}  // namespace neo::cli