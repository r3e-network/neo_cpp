#!/bin/bash
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
"$DIR/bin/neo-node" --config "$DIR/config/testnet.json" "$@"
