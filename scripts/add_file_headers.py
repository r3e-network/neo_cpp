#!/usr/bin/env python3
"""
add_file_headers.py - Add missing @file documentation headers
Automatically adds Doxygen file headers to C++ files missing them.
"""

import os
import re
import sys
from pathlib import Path
from datetime import datetime

def get_brief_description(filename):
    """Generate a brief description based on filename."""
    # Remove extension and path
    name = Path(filename).stem
    
    # Convert snake_case to Title Case
    words = name.replace('_', ' ').split()
    
    # Special cases
    descriptions = {
        'blockchain': 'Core blockchain implementation',
        'consensus_service': 'dBFT consensus service implementation',
        'memory_pool': 'Transaction memory pool management',
        'transaction': 'Transaction types and processing',
        'block': 'Block structure and validation',
        'neo_system': 'Main Neo system coordinator',
        'vm': 'Virtual machine implementation',
        'opcode': 'VM operation codes',
        'application_engine': 'Smart contract execution engine',
        'native_contract': 'Native contract implementations',
        'neo_token': 'NEO governance token contract',
        'gas_token': 'GAS utility token contract',
        'crypto': 'Cryptographic operations',
        'ecc': 'Elliptic curve cryptography',
        'hash': 'Hashing algorithms',
        'merkle_tree': 'Merkle tree implementation',
        'rpc_server': 'JSON-RPC server implementation',
        'p2p': 'Peer-to-peer networking',
        'tcp_connection': 'TCP network connections',
        'message': 'Network message handling',
        'storage': 'Persistent storage management',
        'rocksdb_store': 'RocksDB storage backend',
        'data_cache': 'Data caching layer',
        'serializable': 'Serialization interfaces',
        'json': 'JSON serialization utilities',
        'uint160': '160-bit unsigned integer type',
        'uint256': '256-bit unsigned integer type',
        'fixed8': 'Fixed-point decimal type',
        'logger': 'Logging infrastructure',
        'settings': 'Configuration settings',
        'protocol_settings': 'Protocol configuration',
        'metrics': 'Performance metrics collection',
        'test': 'Unit tests',
        'mock': 'Mock implementations for testing',
        'helper': 'Helper utilities',
        'utils': 'Utility functions',
        'types': 'Type definitions',
        'constants': 'Constant definitions',
        'enums': 'Enumeration definitions',
        'exceptions': 'Exception types',
        'interfaces': 'Interface definitions',
        'factory': 'Factory pattern implementations',
        'manager': 'Management components',
        'service': 'Service implementations',
        'handler': 'Event and message handlers',
        'validator': 'Validation logic',
        'verifier': 'Verification components',
        'builder': 'Builder pattern implementations',
        'pool': 'Resource pooling',
        'cache': 'Caching mechanisms',
        'queue': 'Queue data structures',
        'buffer': 'Buffer management',
        'stream': 'Stream processing',
        'codec': 'Encoding/decoding operations',
        'parser': 'Parsing utilities',
        'formatter': 'Data formatting',
        'converter': 'Type conversion utilities',
        'wrapper': 'Wrapper implementations',
        'adapter': 'Adapter pattern implementations',
        'proxy': 'Proxy pattern implementations',
        'delegate': 'Delegation mechanisms',
        'observer': 'Observer pattern implementations',
        'event': 'Event handling',
        'callback': 'Callback mechanisms',
        'thread': 'Threading utilities',
        'sync': 'Synchronization primitives',
        'lock': 'Locking mechanisms',
        'atomic': 'Atomic operations',
        'config': 'Configuration management',
        'init': 'Initialization logic',
        'startup': 'Startup procedures',
        'shutdown': 'Shutdown procedures',
        'cleanup': 'Cleanup utilities',
    }
    
    # Check for exact matches
    for key, desc in descriptions.items():
        if key in name.lower():
            return desc
    
    # Default: capitalize words
    return ' '.join(word.capitalize() for word in words)

def has_file_header(content):
    """Check if file already has @file documentation."""
    # Check for @file in first 20 lines
    lines = content.split('\n')[:20]
    for line in lines:
        if '@file' in line:
            return True
    return False

def add_file_header(filepath):
    """Add Doxygen file header to a C++ file."""
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Skip if already has header
    if has_file_header(content):
        return False
    
    # Get filename and description
    filename = os.path.basename(filepath)
    brief = get_brief_description(filename)
    
    # Determine if it's a header or source file
    is_header = filepath.endswith('.h') or filepath.endswith('.hpp')
    file_type = "header" if is_header else "implementation"
    
    # Create the header
    header = f'''/**
 * @file {filename}
 * @brief {brief}
 * @author Neo C++ Team
 * @date {datetime.now().year}
 * @copyright MIT License
 */

'''
    
    # Handle different file starts
    if content.startswith('#pragma once'):
        # Header file with pragma once
        content = content.replace('#pragma once', f'{header}#pragma once', 1)
    elif content.startswith('#ifndef'):
        # Header file with include guards
        lines = content.split('\n')
        # Find the #ifndef and #define lines
        guard_lines = []
        for i, line in enumerate(lines[:5]):
            if line.startswith('#ifndef') or line.startswith('#define'):
                guard_lines.append(i)
        
        if len(guard_lines) >= 2:
            # Insert after the guard
            lines.insert(guard_lines[-1] + 1, header.rstrip())
            content = '\n'.join(lines)
        else:
            # Just prepend
            content = header + content
    elif content.startswith('#include'):
        # Source file starting with includes
        content = header + content
    elif content.startswith('//'):
        # File starting with comments - replace them
        lines = content.split('\n')
        # Skip comment lines
        i = 0
        while i < len(lines) and (lines[i].startswith('//') or lines[i].strip() == ''):
            i += 1
        
        # Keep the rest
        content = header + '\n'.join(lines[i:])
    else:
        # Just prepend the header
        content = header + content
    
    # Write back
    with open(filepath, 'w', encoding='utf-8') as f:
        f.write(content)
    
    return True

def process_directory(directory):
    """Process all C++ files in a directory."""
    updated = 0
    skipped = 0
    
    # Find all header and source files
    for root, dirs, files in os.walk(directory):
        # Skip third_party and build directories
        if 'third_party' in root or 'build' in root or '.git' in root:
            continue
        
        for file in files:
            if file.endswith(('.h', '.hpp', '.cpp', '.cc')):
                filepath = os.path.join(root, file)
                
                try:
                    if add_file_header(filepath):
                        print(f"✓ Added header to {filepath}")
                        updated += 1
                    else:
                        skipped += 1
                except Exception as e:
                    print(f"✗ Error processing {filepath}: {e}")
    
    return updated, skipped

def main():
    """Main entry point."""
    if len(sys.argv) > 1:
        directory = sys.argv[1]
    else:
        # Default to include and src directories
        directory = '.'
    
    print("Adding missing file headers...")
    print("")
    
    # Process include directory
    include_updated = 0
    include_skipped = 0
    if os.path.exists('include'):
        print("Processing include directory...")
        u, s = process_directory('include')
        include_updated += u
        include_skipped += s
    
    # Process src directory
    src_updated = 0
    src_skipped = 0
    if os.path.exists('src'):
        print("Processing src directory...")
        u, s = process_directory('src')
        src_updated += u
        src_skipped += s
    
    # Summary
    print("")
    print("=" * 40)
    print("Summary:")
    print(f"  Files updated: {include_updated + src_updated}")
    print(f"  Files skipped: {include_skipped + src_skipped}")
    print("=" * 40)
    
    if include_updated + src_updated > 0:
        print("")
        print("✅ File headers added successfully!")
        print("Run './scripts/check_documentation.sh' to verify coverage.")

if __name__ == '__main__':
    main()