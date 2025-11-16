# Running the Neo C++ Node on Neo N3 Networks

This guide shows how to start the C++ node on the public Neo N3 TestNet or MainNet using the CLI service that now understands network presets.

## 1. Build the CLI

```bash
cmake -S . -B build
cmake --build build --target neo_cli_app
```

The binary will be available at `build/apps/cli/neo_cli_app`.

## 2. Launch on TestNet

```bash
./build/apps/cli/neo_cli_app --network testnet
```

- Loads `config/testnet.config.json` automatically.
- Binds to `0.0.0.0:20333` and connects to the official seed nodes (`seed*.neo.org`).
- Stores blockchain data in `./testnet-data` (from the config file).
- Persists peers in `./testnet-data/peers.dat` (path is printed during startup).

## 3. Launch on MainNet

```bash
./build/apps/cli/neo_cli_app --network mainnet
```

- Loads `config/mainnet.config.json` (network magic `0x334F454E`).
- Binds to `0.0.0.0:10333` and connects to the five official mainnet seeds.
- Uses the data/log paths defined in the config, and persists peers in the same directory.

## 4. Custom Configurations

If you need to run against a custom network or modified config file, pass the file directly and optionally override the storage path:

```bash
./build/apps/cli/neo_cli_app \
    --config my_config.json \
    --db-path /data/neo-node
```

Command-line overrides for `--db-engine` and `--db-path` still apply after the configuration file is loaded.

## 5. Monitoring Synchronization

Once the CLI is running, the console prints synchronization state changes:

```
Synchronization: Synchronizing headers
Synchronization: Synchronizing blocks
Synchronization: Synchronized
```

You can run the `show state` command inside the CLI to check peer counts and block height once the node is connected.

## 6. Securing the RPC Server

The CLI reads RPC authentication and TLS settings directly from the JSON configuration:

- Add `Username` / `Password` under `ApplicationConfiguration.RPC` to enforce HTTP Basic auth.
- Set `EnableSsl` to `true` alongside `SslCert` (PEM chain) and `SslKey` (private key) to serve HTTPS.
- Optional fields: `TrustedAuthorities` (array of client CA PEM files, enabling mutual TLS),
  `SslCiphers` (OpenSSL cipher string), and `MinTlsVersion` (e.g., `"1.2"`).

Example:

```json
"ApplicationConfiguration": {
  "RPC": {
    "Enabled": true,
    "BindAddress": "127.0.0.1",
    "Port": 20332,
    "Username": "neo",
    "Password": "changeme",
    "EnableSsl": true,
    "SslCert": "/etc/ssl/certs/neo.crt",
    "SslKey": "/etc/ssl/private/neo.key"
  }
}
```

No additional CLI flags are needed—the RPC server automatically enables auth/TLS based on these fields.

## 7. Using the Production Wrapper

If you prefer the production `neo_node_complete` harness:

```bash
cmake --build build --target neo_node
./build/apps/neo_node/neo_node --network testnet

# Or explicitly:
# ./build/apps/neo_node/neo_node --config config/testnet.config.json
```

The new peer list path logging is also available there, pointing at the resolved `peers.dat` location under the configured data directory.

---
These steps bring the C++ node up against the public Neo N3 networks without manual configuration edits.
