#pragma once

#include <neo/smartcontract/application_engine.h>

namespace neo::smartcontract
{
    /**
     * @brief Registers all runtime-related system calls.
     * @param engine The application engine to register the system calls with.
     */
    void RegisterRuntimeSystemCalls(ApplicationEngine& engine);

    /**
     * @brief Registers all storage-related system calls.
     * @param engine The application engine to register the system calls with.
     */
    void RegisterStorageSystemCalls(ApplicationEngine& engine);

    /**
     * @brief Registers all contract-related system calls.
     * @param engine The application engine to register the system calls with.
     */
    void RegisterContractSystemCalls(ApplicationEngine& engine);

    /**
     * @brief Registers all crypto-related system calls.
     * @param engine The application engine to register the system calls with.
     */
    void RegisterCryptoSystemCalls(ApplicationEngine& engine);

    /**
     * @brief Registers all JSON-related system calls.
     * @param engine The application engine to register the system calls with.
     */
    void RegisterJsonSystemCalls(ApplicationEngine& engine);
}
