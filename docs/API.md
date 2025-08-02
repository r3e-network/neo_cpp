# Neo C++ API Documentation

## JSON-RPC API

The Neo C++ node provides a JSON-RPC 2.0 API compatible with the official Neo N3 specification.

### Endpoint

- **HTTP**: `http://localhost:10334`
- **HTTPS**: `https://localhost:10335` (if TLS enabled)
- **WebSocket**: `ws://localhost:10336` (for subscriptions)

### Request Format

```json
{
    "jsonrpc": "2.0",
    "method": "method_name",
    "params": [...],
    "id": 1
}
```

### Response Format

```json
{
    "jsonrpc": "2.0",
    "result": ...,
    "id": 1
}
```

### Error Response

```json
{
    "jsonrpc": "2.0",
    "error": {
        "code": -32000,
        "message": "Error description",
        "data": "Additional information"
    },
    "id": 1
}
```

## Methods

### Blockchain Methods

#### getbestblockhash

Get the hash of the latest block.

**Parameters**: None

**Returns**: `string` - The hash of the latest block

**Example**:
```bash
curl -X POST http://localhost:10334 \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","method":"getbestblockhash","params":[],"id":1}'
```

**Response**:
```json
{
    "jsonrpc": "2.0",
    "result": "0x3f1c7d14c86c6f8e9c9cbf97b3b6c8c4c2c8e06f0dc9c1f8a9f2c8f7e9b8a7f6",
    "id": 1
}
```

#### getblock

Get block information by hash or index.

**Parameters**:
- `hash_or_index`: `string|number` - Block hash or index
- `verbose`: `boolean` - Optional, default: true. If false, returns Base64 string

**Returns**: Block object or Base64 string

**Example**:
```bash
curl -X POST http://localhost:10334 \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","method":"getblock","params":[1000, true],"id":1}'
```

**Response**:
```json
{
    "jsonrpc": "2.0",
    "result": {
        "hash": "0x...",
        "size": 1217,
        "version": 0,
        "previousblockhash": "0x...",
        "merkleroot": "0x...",
        "time": 1468595301000,
        "nonce": "158588e1ea2b88cf",
        "index": 1000,
        "primary": 0,
        "nextconsensus": "NXjtqYERuvSWGawjVux8UerNejvwdYg7eE",
        "witnesses": [...],
        "tx": [...],
        "confirmations": 12345,
        "nextblockhash": "0x..."
    },
    "id": 1
}
```

#### getblockcount

Get the current block height.

**Parameters**: None

**Returns**: `number` - The current block height

**Example**:
```bash
curl -X POST http://localhost:10334 \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","method":"getblockcount","params":[],"id":1}'
```

#### getblockhash

Get block hash by index.

**Parameters**:
- `index`: `number` - Block index

**Returns**: `string` - Block hash

#### getblockheader

Get block header information.

**Parameters**:
- `hash_or_index`: `string|number` - Block hash or index
- `verbose`: `boolean` - Optional, default: true

**Returns**: Block header object or Base64 string

#### getblockheadercount

Get the current block header height.

**Parameters**: None

**Returns**: `number` - The current block header height

### Transaction Methods

#### getrawtransaction

Get transaction information.

**Parameters**:
- `txid`: `string` - Transaction ID
- `verbose`: `boolean` - Optional, default: true

**Returns**: Transaction object or Base64 string

**Example**:
```bash
curl -X POST http://localhost:10334 \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","method":"getrawtransaction","params":["0x..."],"id":1}'
```

#### sendrawtransaction

Submit a new transaction.

**Parameters**:
- `hex`: `string` - Base64 encoded transaction

**Returns**: `boolean` - Success status

**Example**:
```bash
curl -X POST http://localhost:10334 \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","method":"sendrawtransaction","params":["AAAAAACYloU..."],"id":1}'
```

#### gettransactionheight

Get the block height of a transaction.

**Parameters**:
- `txid`: `string` - Transaction ID

**Returns**: `number` - Block height

### Smart Contract Methods

#### invokefunction

Invoke a smart contract method.

**Parameters**:
- `scripthash`: `string` - Contract script hash
- `operation`: `string` - Method name
- `params`: `array` - Optional, method parameters
- `signers`: `array` - Optional, transaction signers

**Returns**: Invocation result object

**Example**:
```bash
curl -X POST http://localhost:10334 \
  -H "Content-Type: application/json" \
  -d '{
    "jsonrpc": "2.0",
    "method": "invokefunction",
    "params": [
        "0xef4073a0f2b305a38ec4050e4d3d28bc40ea63f5",
        "symbol",
        []
    ],
    "id": 1
}'
```

**Response**:
```json
{
    "jsonrpc": "2.0",
    "result": {
        "script": "10c00c067379...",
        "state": "HALT",
        "gasconsumed": "2007570",
        "exception": null,
        "stack": [
            {
                "type": "ByteString",
                "value": "TkVP"
            }
        ]
    },
    "id": 1
}
```

#### invokescript

Execute a script in the VM.

**Parameters**:
- `script`: `string` - Base64 encoded script
- `signers`: `array` - Optional, transaction signers

**Returns**: Invocation result object

#### getcontractstate

Get contract information.

**Parameters**:
- `scripthash`: `string` - Contract script hash

**Returns**: Contract state object

**Example Response**:
```json
{
    "id": -5,
    "updatecounter": 0,
    "hash": "0xef4073a0f2b305a38ec4050e4d3d28bc40ea63f5",
    "nef": {
        "magic": 860243278,
        "compiler": "neo-core-v3.0",
        "source": "",
        "tokens": [],
        "script": "EEEa93tn..."
    },
    "manifest": {
        "name": "NeoToken",
        "groups": [],
        "features": {},
        "supportedstandards": ["NEP-17"],
        "abi": {...},
        "permissions": [...],
        "trusts": [],
        "extra": null
    }
}
```

### Node Methods

#### getconnectioncount

Get the number of connected peers.

**Parameters**: None

**Returns**: `number` - Connected peer count

#### getpeers

Get information about connected peers.

**Parameters**: None

**Returns**: Object with connected and unconnected peers

**Example Response**:
```json
{
    "unconnected": [],
    "bad": [],
    "connected": [
        {
            "address": "127.0.0.1:10333",
            "port": 10333
        }
    ]
}
```

#### getversion

Get node version information.

**Parameters**: None

**Returns**: Version object

**Example Response**:
```json
{
    "tcpport": 10333,
    "wsport": 10334,
    "nonce": 1234567890,
    "useragent": "/Neo:3.6.0/",
    "protocol": {
        "addressversion": 53,
        "network": 860833102,
        "validatorscount": 7,
        "msperblock": 15000,
        "maxtraceableblocks": 2102400,
        "maxvaliduntilblockincrement": 5760,
        "maxtransactionsperblock": 512,
        "memorypoolmaxtransactions": 50000,
        "initialgasdistribution": 5200000000000000
    }
}
```

### State Methods

#### getproof

Get a state proof.

**Parameters**:
- `roothash`: `string` - State root hash
- `scripthash`: `string` - Contract script hash
- `key`: `string` - Storage key

**Returns**: Proof string

#### getstateheight

Get the current state height.

**Parameters**: None

**Returns**: Object with local root index and validated root index

#### getstateroot

Get state root by index.

**Parameters**:
- `index`: `number` - State root index

**Returns**: State root object

#### getstorage

Get storage value.

**Parameters**:
- `scripthash`: `string` - Contract script hash
- `key`: `string` - Base64 encoded storage key

**Returns**: `string` - Base64 encoded value

**Example**:
```bash
curl -X POST http://localhost:10334 \
  -H "Content-Type: application/json" \
  -d '{
    "jsonrpc": "2.0",
    "method": "getstorage",
    "params": [
        "0xef4073a0f2b305a38ec4050e4d3d28bc40ea63f5",
        "VG90YWxTdXBwbHk="
    ],
    "id": 1
}'
```

#### findstates

Find contract storage by prefix.

**Parameters**:
- `roothash`: `string` - State root hash
- `scripthash`: `string` - Contract script hash
- `prefix`: `string` - Base64 encoded prefix
- `from`: `string` - Optional, Base64 encoded start key
- `count`: `number` - Optional, default: all

**Returns**: Array of results

### Utility Methods

#### calculatenetworkfee

Calculate network fee for a transaction.

**Parameters**:
- `tx`: `string` - Base64 encoded transaction

**Returns**: `string` - Network fee amount

#### getnep17balances

Get NEP-17 token balances for an address.

**Parameters**:
- `address`: `string` - Address

**Returns**: Object with balance array and address

#### getnep17transfers

Get NEP-17 token transfers for an address.

**Parameters**:
- `address`: `string` - Address
- `startTime`: `number` - Optional, start timestamp
- `endTime`: `number` - Optional, end timestamp

**Returns**: Object with sent and received arrays

#### validateaddress

Validate an address format.

**Parameters**:
- `address`: `string` - Address to validate

**Returns**: Object with validation result

**Example Response**:
```json
{
    "address": "NXjtqYERuvSWGawjVux8UerNejvwdYg7eE",
    "isvalid": true
}
```

## WebSocket API

The WebSocket API allows real-time subscriptions to blockchain events.

### Connection

```javascript
const ws = new WebSocket('ws://localhost:10336');
```

### Subscribe to New Blocks

```json
{
    "jsonrpc": "2.0",
    "method": "subscribe",
    "params": ["newblock"],
    "id": 1
}
```

### Subscribe to New Transactions

```json
{
    "jsonrpc": "2.0",
    "method": "subscribe",
    "params": ["newtx"],
    "id": 2
}
```

### Subscribe to Contract Events

```json
{
    "jsonrpc": "2.0",
    "method": "subscribe",
    "params": ["notification", "0xef4073a0f2b305a38ec4050e4d3d28bc40ea63f5"],
    "id": 3
}
```

### Unsubscribe

```json
{
    "jsonrpc": "2.0",
    "method": "unsubscribe",
    "params": ["subscription_id"],
    "id": 4
}
```

## Error Codes

| Code | Message | Description |
|------|---------|-------------|
| -32700 | Parse error | Invalid JSON |
| -32600 | Invalid request | Invalid request format |
| -32601 | Method not found | Unknown method |
| -32602 | Invalid params | Invalid parameters |
| -32603 | Internal error | Internal server error |
| -32000 | General error | Application-specific error |

## Rate Limiting

The API implements rate limiting to prevent abuse:

- **Default limit**: 100 requests per minute per IP
- **Burst limit**: 200 requests
- **Headers**: `X-RateLimit-Limit`, `X-RateLimit-Remaining`, `X-RateLimit-Reset`

## Authentication

For private methods, use API key authentication:

```bash
curl -X POST http://localhost:10334 \
  -H "Content-Type: application/json" \
  -H "X-API-Key: your-api-key" \
  -d '{"jsonrpc":"2.0","method":"private_method","params":[],"id":1}'
```

## SDK Examples

### C++ SDK

```cpp
#include <neo/sdk/rpc_client.h>

neo::sdk::RpcClient client("http://localhost:10334");

// Get block count
auto count = client.GetBlockCount();
std::cout << "Block height: " << count << std::endl;

// Get block
auto block = client.GetBlock(1000);
std::cout << "Block hash: " << block.hash << std::endl;

// Invoke contract
auto result = client.InvokeFunction(
    "0xef4073a0f2b305a38ec4050e4d3d28bc40ea63f5",
    "symbol",
    {}
);
```

### Python SDK

```python
from neo_sdk import RpcClient

client = RpcClient("http://localhost:10334")

# Get block count
count = client.get_block_count()
print(f"Block height: {count}")

# Get block
block = client.get_block(1000)
print(f"Block hash: {block['hash']}")

# Invoke contract
result = client.invoke_function(
    "0xef4073a0f2b305a38ec4050e4d3d28bc40ea63f5",
    "symbol",
    []
)
```

### JavaScript SDK

```javascript
const { RpcClient } = require('neo-sdk');

const client = new RpcClient('http://localhost:10334');

// Get block count
const count = await client.getBlockCount();
console.log(`Block height: ${count}`);

// Get block
const block = await client.getBlock(1000);
console.log(`Block hash: ${block.hash}`);

// Invoke contract
const result = await client.invokeFunction(
    '0xef4073a0f2b305a38ec4050e4d3d28bc40ea63f5',
    'symbol',
    []
);
```