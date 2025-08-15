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

#### getbestblockhash
Returns the hash of the best (latest) block.

### 3. Transaction Methods

#### getrawtransaction
Returns transaction information.

**Parameters:**
- `txid`: Transaction ID
- `verbose`: Optional boolean

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
Broadcasts a transaction to the network.

**Parameters:**
- `hex`: Signed transaction in hex format

**Request:**
```json
{
  "jsonrpc": "2.0",
  "method": "sendrawtransaction",
  "params": ["00d1001500..."],
  "id": 1
}
```

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

#### getnep17balances
Returns NEP-17 token balances for an address.

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

#### getnep17transfers
Returns NEP-17 transfer history for an address.

**Parameters:**
- `address`: Neo address
- `starttime`: Optional start timestamp
- `endtime`: Optional end timestamp

### 6. State Methods

#### getstorage
Returns storage value from a contract.

**Parameters:**
- `scripthash`: Contract script hash
- `key`: Storage key in hex

#### findstorage
Finds storage entries by prefix.

**Parameters:**
- `scripthash`: Contract script hash
- `prefix`: Key prefix in hex
- `start`: Optional start index

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

### 8. Network Methods

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
Returns transaction hashes in the memory pool.

**Parameters:**
- `shouldgetunverified`: Optional boolean

### 9. Utility Methods

#### listplugins
Returns loaded plugins.

#### calculatenetworkfee
Calculates network fee for a transaction.

**Parameters:**
- `tx`: Transaction in base64

#### getapplicationlog
Returns application execution log.

**Parameters:**
- `txid`: Transaction ID

### 10. Native Contract Methods

#### getnativecontracts
Returns information about native contracts.

**Response:**
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": [
    {
      "id": -1,
      "hash": "0xef4073a0f2b305a38ec4050e4d3d28bc40ea63f5",
      "name": "NeoToken",
      "manifest": {...}
    },
    {
      "id": -2,
      "hash": "0xd2a4cff31913016155e38e474a2c06d08be276cf",
      "name": "GasToken",
      "manifest": {...}
    }
  ]
}
```

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