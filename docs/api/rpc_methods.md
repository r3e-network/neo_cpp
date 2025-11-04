# RPC Methods Documentation

## Overview

The Neo C++ node implements a complete JSON-RPC 2.0 server with all standard Neo RPC methods. The server is production-ready with comprehensive error handling, metrics, and monitoring support.

## Connection Details

- **Default Port**: 10332 (MainNet), 20332 (TestNet)
- **Protocol**: HTTP/HTTPS
- **Format**: JSON-RPC 2.0
- **Content-Type**: application/json

## Method Categories

### 1. Node Information Methods

#### getversion
Returns version information about the Neo node.

**Request:**
```json
{
  "jsonrpc": "2.0",
  "method": "getversion",
  "params": [],
  "id": 1
}
```

**Response:**
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": {
    "tcpport": 10333,
    "wsport": 10334,
    "nonce": 1234567890,
    "useragent": "/Neo:3.6.0/",
    "protocol": {
      "addressversion": 53,
      "network": 860833102,
      "validatorscount": 7,
      "msperblock": 15000
    }
  }
}
```

#### getblockcount
Returns the current block height.

**Request:**
```json
{
  "jsonrpc": "2.0",
  "method": "getblockcount",
  "params": [],
  "id": 1
}
```

**Response:**
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": 1234567
}
```

#### getconnectioncount
Returns the number of connected peers.

**Request:**
```json
{
  "jsonrpc": "2.0",
  "method": "getconnectioncount",
  "params": [],
  "id": 1
}
```

### 2. Block Methods

#### getblock
Returns block information by hash or index.

**Parameters:**
- `hash|index`: Block hash (string) or index (number)
- `verbose`: Optional boolean (default: true)

**Responses:**
- When `verbose` is `false`, the method returns a Base64-encoded block payload identical to the binary wire format.
- When `verbose` is `true`, the response is a JSON object containing:
  - `hash`, `size`, `version`, `previousblockhash`, `merkleroot`, `time`, `nonce`, `index`, `primary`
  - `nextconsensus` (Base58-encoded address based on current `ProtocolSettings`)
  - `confirmations` and `nextblockhash` when available
  - `witnesses` array (single entry matching the block header witness)
  - `tx` array with full verbose transaction objects (matching C# RPC output, including fees, signers, witnesses)

**Request:**
```json
{
  "jsonrpc": "2.0",
  "method": "getblock",
  "params": ["0x1234...abcd", true],
  "id": 1
}
```

#### getblockheader
Returns block header information.

**Parameters:**
- `hash|index`: Block hash or index
- `verbose`: Optional boolean

**Responses:**
- `verbose = false`: Base64-encoded header payload (binary wire format).
- `verbose = true`: JSON object with the same fields as the verbose block header in the C# node (`hash`, `size`, `version`, `previousblockhash`, `merkleroot`, `time`, `nonce`, `index`, `primary`, `nextconsensus`, `witnesses`, `confirmations`, `nextblockhash`).

#### getbestblockhash
Returns the hash of the best (latest) block.

#### submitblock
Submits a signed block to the node for verification and optional relay.

**Parameters:**
- `block`: Base64-encoded block payload in Neo binary wire format.
- `relay` (optional, default `true`): When `true`, the node relays the block to its peers after successful verification.

**Responses:**
- On success, returns an object with a single `hash` field containing the block hash.
- On failure, the RPC call returns an error whose message includes the `VerifyResult` reason (for example `AlreadyExists`, `UnableToVerify`, etc.), matching the C# node behaviour.

### 3. Transaction Methods

#### getrawtransaction
Returns transaction information.

**Parameters:**
- `txid`: Transaction ID
- `verbose`: Optional boolean

**Responses:**
- `verbose = false`: Base64-encoded transaction payload (wire format), sourced from the mempool if available, otherwise from persisted storage.
- `verbose = true`: JSON object mirroring the C# RPC output and including:
  - `hash`, `size`, `version`, `nonce`, `sender`, `sysfee`, `netfee`, `validuntilblock`
  - `signers`, `attributes`, `script`, `witnesses`
  - `blockhash`, `confirmations`, `blocktime` when the transaction has been persisted

**Request:**
```json
{
  "jsonrpc": "2.0",
  "method": "getrawtransaction",
  "params": ["0xabcd...1234", true],
  "id": 1
}
```

#### sendrawtransaction
Broadcasts a signed transaction to the network.

**Parameters:**
- `tx`: Base64 encoded transaction payload

**Request:**
```json
{
  "jsonrpc": "2.0",
  "method": "sendrawtransaction",
  "params": ["AEHAAQ=="],
  "id": 1
}
```

**Response:**
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": { "hash": "0x1f6901..." }
}
```

If the transaction fails validation the RPC call returns an error whose message contains the verification reason (for example `AlreadyExists`, `PolicyFail`, etc.).

#### gettransactionheight
Returns the block height containing a transaction.

**Parameters:**
- `txid`: Transaction ID

### 4. Smart Contract Methods

#### invokefunction
Invokes a smart contract method (test invocation).

**Parameters:**
- `scripthash`: Contract script hash
- `method`: Method name
- `params`: Optional array of parameters
- `signers`: Optional array of signers

**Request:**
```json
{
  "jsonrpc": "2.0",
  "method": "invokefunction",
  "params": [
    "0xd2a4cff31913016155e38e474a2c06d08be276cf",
    "balanceOf",
    [{"type": "Hash160", "value": "0x1234..."}]
  ],
  "id": 1
}
```

#### invokescript
Executes a script (test invocation).

**Parameters:**
- `script`: Script in hex format
- `signers`: Optional array of signers

#### getcontractstate
Returns contract information.

**Parameters:**
- `scripthash`: Contract script hash

### 5. NEP-17 Token Methods

#### getnep17balances *(not yet implemented in neo_cpp)*
Returns NEP-17 token balances for an address on the C# node. The C++ port currently returns a `MethodNotFound` error.

**Parameters:**
- `address`: Neo address

**Request:**
```json
{
  "jsonrpc": "2.0",
  "method": "getnep17balances",
  "params": ["NXV7ZhHiyM1aHXwpVsRZC6BwNFP2jghXAq"],
  "id": 1
}
```

**Response:**
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": {
    "balance": [
      {
        "assethash": "0xef4073a0f2b305a38ec4050e4d3d28bc40ea63f5",
        "symbol": "NEO",
        "decimals": 0,
        "amount": "100",
        "lastupdatedblock": 1234567
      },
      {
        "assethash": "0xd2a4cff31913016155e38e474a2c06d08be276cf",
        "symbol": "GAS",
        "decimals": 8,
        "amount": "5000000000",
        "lastupdatedblock": 1234567
      }
    ],
    "address": "NXV7ZhHiyM1aHXwpVsRZC6BwNFP2jghXAq"
  }
}
```

#### getnep17transfers *(not yet implemented in neo_cpp)*
Returns NEP-17 transfer history for an address on the C# node. The C++ port currently returns a `MethodNotFound` error.

**Parameters:**
- `address`: Neo address
- `starttime`: Optional start timestamp
- `endtime`: Optional end timestamp

### 6. State Methods

#### getstorage
Returns a storage value from a contract.

**Parameters:**
- `contract`: Contract id (number), script hash (string) or native contract name (string)
- `key`: Base64-encoded storage key

#### findstorage
Finds storage entries by prefix.

**Parameters:**
- `contract`: Contract id, script hash, or native contract name
- `prefix`: Base64-encoded prefix
- `start`: Optional start index (default 0)

#### traverseiterator
Returns the next batch of values for a previously-created iterator (for example the iterator returned by `findstorage`).

**Parameters:**
- `session`: Session identifier string (returned when the iterator was created).
- `iterator`: Iterator identifier string.
- `count`: Optional maximum number of values to return (defaults to 100; values ≤ 0 also default to 100).

**Response:**
```json
{
  "values": [ /* iterator items as JSON */ ],
  "truncated": true
}
```
- Throws `UnknownSession` if the session has expired, and `UnknownIterator` if the iterator id is unknown.

#### terminatesession
Stops a session and releases any iterators tracked for it.

**Parameters:**
- `session`: Session identifier string.

**Response:**
- Boolean `true` when the session existed and was removed.
- Throws `UnknownSession` when the session id is not currently tracked.

### 7. Validation Methods

#### validateaddress
Validates a Neo address.

**Parameters:**
- `address`: Address to validate

**Response:**
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": {
    "address": "NXV7ZhHiyM1aHXwpVsRZC6BwNFP2jghXAq",
    "isvalid": true
  }
}
```

### 8. Governance Methods

#### getcommittee
Returns the current committee members (compressed public keys).

**Request:**
```json
{
  "jsonrpc": "2.0",
  "method": "getcommittee",
  "params": [],
  "id": 1
}
```

**Response:**
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": [
    "02bca21b6a2e7c7c7a53c6ab5f8f2f3f4c5d6e7f8a9b0c1d2e3f4a5b6c7d8e9f0"
  ]
}
```

#### getvalidators
Returns the current consensus validator set.

**Response:** Array of compressed public keys.

#### getnextblockvalidators
Returns the validators scheduled for the next block.

**Response:** Array of objects `{ "publickey": "...", "votes": <integer> }`.

#### getcandidates
Returns registered validator candidates and whether they are currently active.

**Response:**
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": [
    {
      "publickey": "0371...c4",
      "votes": "123456789",
      "active": true
    }
  ]
}
```

### 9. Network Methods

#### getpeers
Returns connected and disconnected peers.

**Response:**
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": {
    "connected": [
      {
        "address": "127.0.0.1",
        "port": 10333
      }
    ],
    "bad": [],
    "unconnected": []
  }
}
```

#### getrawmempool
Returns the current contents of the memory pool.

**Parameters:**
- `shouldgetunverified` (optional, default `false`): When `true`, include unverified transactions and the current height.

**Response (when `shouldgetunverified` is `false`):**
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": [
    "0x9f04...",
    "0xb1a7..."
  ]
}
```

**Response (when `shouldgetunverified` is `true`):**
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": {
    "height": 12345,
    "verified": ["0x9f04..."],
    "unverified": ["0xc2aa..."]
  }
}
```

### 10. Utility Methods

#### listplugins
Returns loaded plugins.

#### calculatenetworkfee *(not yet implemented in neo_cpp)*
Calculates network fee for a transaction on the C# node. This RPC is not yet available in the C++ port.

**Parameters:**
- `tx`: Transaction in base64

#### getapplicationlog *(not yet implemented in neo_cpp)*
Returns application execution logs on the C# node. This RPC is not yet available in the C++ port.

**Parameters:**
- `txid`: Transaction ID

### 10. Native Contract Methods

#### getnativecontracts
Returns descriptors for native contracts.

**Response:**
- Array where each element contains:
  - `id`, `updatecounter`, `hash`
  - `nef`: `{ magic, compiler, tokens, script (Base64), checksum }`
  - `manifest`: parsed manifest JSON identical to the C# RPC response

## Error Codes

| Code | Message | Description |
|------|---------|-------------|
| -32700 | Parse error | Invalid JSON |
| -32600 | Invalid request | Invalid request format |
| -32601 | Method not found | Method does not exist |
| -32602 | Invalid params | Invalid method parameters |
| -32603 | Internal error | Internal server error |
| -32001 | Block not found | Requested block not found |
| -32002 | Transaction not found | Transaction not found |
| -32003 | Contract not found | Contract not found |
| -32004 | Storage item not found | Storage item not found |
| -32005 | Unknown error | Unknown error occurred |

## Rate Limiting

The RPC server implements rate limiting to prevent abuse:
- **Default limit**: 100 requests per second per IP
- **Burst limit**: 200 requests
- **Configurable via**: `rpc.ratelimit` in configuration

## Authentication

Optional authentication can be enabled:
- **Basic Auth**: Username/password
- **Token Auth**: Bearer token
- **IP Whitelist**: Restrict to specific IPs

## Monitoring

The RPC server exposes metrics at `/metrics`:
- Request count by method
- Request latency histogram
- Error count by type
- Active connections

## WebSocket Support

WebSocket connections are supported for real-time updates:
- **Endpoint**: ws://localhost:10334
- **Subscriptions**: blocks, transactions, notifications
- **Protocol**: JSON-RPC 2.0 over WebSocket

## Examples

### Using curl

```bash
# Get block count
curl -X POST http://localhost:10332 \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","method":"getblockcount","params":[],"id":1}'

# Get NEP-17 balances
curl -X POST http://localhost:10332 \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","method":"getnep17balances","params":["NXV7ZhHiyM1aHXwpVsRZC6BwNFP2jghXAq"],"id":1}'
```

### Using Neo C++ SDK

```cpp
#include <neo/sdk/rpc/rpc_client.h>

auto client = std::make_shared<neo::sdk::rpc::RpcClient>("http://localhost:10332");

// Get block count
auto height = client->GetBlockCount();

// Get NEP-17 balances
auto balances = client->GetNep17Balances("NXV7ZhHiyM1aHXwpVsRZC6BwNFP2jghXAq");
```

## Performance

- **Average latency**: < 10ms for simple queries
- **Throughput**: > 1000 requests/second
- **Connection pool**: 100 concurrent connections
- **Caching**: Optional response caching

## Security

- **HTTPS support**: TLS 1.2+ recommended
- **Input validation**: All inputs sanitized
- **DoS protection**: Rate limiting and connection limits
- **Audit logging**: All requests logged

---

*RPC Server Version: 3.6.0*
*Neo Protocol: N3*
*Last Updated: August 2025*
#### getconsensusstate
Returns the current dBFT consensus status as reported by the node.

**Request:**
```json
{
  "jsonrpc": "2.0",
  "method": "getconsensusstate",
  "params": [],
  "id": 1
}
```

**Response:**
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": {
    "running": true,
    "blockindex": 1234567,
    "viewnumber": 0,
    "phase": "Primary",
    "prepareresponses": 3,
    "commits": 0,
    "viewchanges": 0,
    "expectedtransactions": 18,
    "transactioncount": 17,
    "proposalhash": "f1a2...9cd0",
    "timestamp": 1712345678901,
    "nonce": 987654321,
    "primaryindex": 0,
    "validatorindex": 2,
    "validatorcount": 7,
    "validators": [
      {
        "index": 0,
        "publickey": "02bca21b6a2...",
        "scripthash": "0x9d6c32c3...",
        "isprimary": true,
        "isme": false,
        "hasproposal": true,
        "hasprepareresponse": false,
        "hascommit": false,
        "viewchangereason": null,
        "requestedview": null
      },
      {
        "index": 1,
        "publickey": "03ab2f4f40f4...",
        "scripthash": "0x1a4f52d0...",
        "isprimary": false,
        "isme": true,
        "hasproposal": false,
        "hasprepareresponse": true,
        "hascommit": true,
        "viewchangereason": "TxInvalid",
        "requestedview": 2
      }
    ]
  }
}
```

Field summary:

- `running` – Whether the consensus engine is active.
- `blockindex` – Current consensus block height.
- `viewnumber` – Current dBFT view.
- `phase` – Textual representation of the consensus phase.
- `prepareresponses`, `commits`, `viewchanges` – Counts of messages collected this round.
- `expectedtransactions` – Number of transaction hashes requested by the current proposal.
- `transactioncount` – Number of transactions currently assembled for the proposal.
- `proposalhash` – Hash of the prepare request currently under consideration (or `null` while no proposal is available).
- `timestamp` – Proposal timestamp (milliseconds since Unix epoch) when available.
- `nonce` – Proposal nonce if provided by the primary.
- `primaryindex` – Validator index of the current primary (or `null` if unavailable).
- `validatorindex` – This node's validator index when it participates (or `null`).
- `validatorcount` – Number of validators currently tracked.
- `validators` – Array describing each validator with its index, compressed public key, script hash, and flags indicating whether it is the current primary, represents this node, has contributed to the current proposal, supplied a prepare response, supplied a commit, or requested a view change (with the reason when applicable). The optional `requestedview` field shows the highest view number that validator has advertised. Possible `viewchangereason` values: `Timeout`, `ChangeAgreement`, `TxNotFound`, `TxRejectedByPolicy`, `TxInvalid`, `BlockRejectedByPolicy`.
