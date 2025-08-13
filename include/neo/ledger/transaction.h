/**
 * @file transaction.h
 * @brief Neo N3 transaction type definitions and interfaces
 * @details This file provides the main Transaction type alias and related
 *          interfaces for Neo N3 blockchain transactions. The implementation
 *          uses the Neo N3 transaction format which includes witness support,
 *          system fees, and network fees.
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/ledger/neo3_transaction_alias.h>

namespace neo::ledger
{
/**
 * @typedef Transaction
 * @brief Main transaction type for Neo N3 blockchain
 * @details Alias for Neo3Transaction which includes:
 *          - Version byte for transaction format
 *          - Nonce for uniqueness
 *          - System fee for computational resources
 *          - Network fee for transaction priority
 *          - Valid until block for expiration
 *          - Signers with witness scopes
 *          - Attributes for additional metadata
 *          - Script containing operations
 *          - Witnesses for authentication
 */
using Transaction = network::p2p::payloads::Neo3Transaction;

/**
 * @typedef IInventory
 * @brief Interface for inventory items (blocks, transactions, etc.)
 * @details Base interface for all items that can be transmitted over the network
 */
using IInventory = network::p2p::payloads::IInventory;

/**
 * @typedef IVerifiable
 * @brief Interface for verifiable items requiring witnesses
 * @details Base interface for items that require cryptographic verification
 */
using IVerifiable = network::p2p::payloads::IVerifiable;

/**
 * @typedef InventoryType
 * @brief Enumeration of inventory types in the Neo protocol
 * @details Specifies the type of inventory item (TX, Block, Consensus, etc.)
 */
using InventoryType = network::p2p::InventoryType;

}  // namespace neo::ledger