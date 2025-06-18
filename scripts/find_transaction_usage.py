#!/usr/bin/env python3
"""
Script to find all usages of the old Transaction class in the Neo C++ codebase.
This will help identify all files that need to be updated to use Neo3Transaction.
"""

import os
import re
from pathlib import Path
from collections import defaultdict

# Patterns to search for
PATTERNS = [
    # Direct Transaction usage
    (r'\bTransaction\b(?!Attribute|Output|Pool|Router|Payload|Storage|Verification)', 'Transaction class usage'),
    (r'std::shared_ptr<Transaction>', 'Transaction shared_ptr'),
    (r'std::vector<Transaction>', 'Transaction vector'),
    (r'std::unique_ptr<Transaction>', 'Transaction unique_ptr'),
    (r'\btransaction\.h\b', 'Transaction header include'),
    
    # Old transaction components that need removal
    (r'\bCoinReference\b', 'CoinReference (Neo 2.x)'),
    (r'\bTransactionOutput\b', 'TransactionOutput (Neo 2.x)'),
    (r'\btransaction->inputs\b', 'Transaction inputs (Neo 2.x)'),
    (r'\btransaction->outputs\b', 'Transaction outputs (Neo 2.x)'),
    (r'\btx->inputs\b', 'Transaction inputs (Neo 2.x)'),
    (r'\btx->outputs\b', 'Transaction outputs (Neo 2.x)'),
    (r'\bTransactionType\b', 'TransactionType enum (Neo 2.x)'),
]

# Directories to search
SEARCH_DIRS = ['include', 'src', 'tests', 'examples', 'apps', 'tools']

# File extensions to search
EXTENSIONS = ['.h', '.hpp', '.cpp', '.cc', '.cxx']

def should_skip_file(filepath):
    """Check if file should be skipped."""
    skip_patterns = [
        'neo3_transaction',  # Already Neo3 compatible
        'build/',
        'vcpkg/',
        '.git/',
        'neo/',  # C# reference
    ]
    return any(pattern in str(filepath) for pattern in skip_patterns)

def search_file(filepath):
    """Search a single file for transaction usage patterns."""
    if should_skip_file(filepath):
        return []
    
    results = []
    try:
        with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()
            lines = content.split('\n')
            
            for pattern, description in PATTERNS:
                for i, line in enumerate(lines, 1):
                    if re.search(pattern, line):
                        results.append({
                            'file': filepath,
                            'line': i,
                            'content': line.strip(),
                            'pattern': description
                        })
    except Exception as e:
        print(f"Error reading {filepath}: {e}")
    
    return results

def main():
    """Main function to find all transaction usages."""
    print("Searching for Transaction class usage in Neo C++ codebase...")
    print("=" * 80)
    
    all_results = defaultdict(list)
    total_files = 0
    affected_files = set()
    
    # Search all relevant directories
    for search_dir in SEARCH_DIRS:
        if not os.path.exists(search_dir):
            continue
            
        for root, _, files in os.walk(search_dir):
            for file in files:
                if any(file.endswith(ext) for ext in EXTENSIONS):
                    filepath = os.path.join(root, file)
                    total_files += 1
                    
                    results = search_file(filepath)
                    if results:
                        affected_files.add(filepath)
                        for result in results:
                            all_results[result['pattern']].append(result)
    
    # Print summary by pattern type
    print("\nSUMMARY BY PATTERN TYPE:")
    print("-" * 80)
    for pattern_desc, results in sorted(all_results.items()):
        print(f"\n{pattern_desc}: {len(results)} occurrences")
        
        # Group by file
        by_file = defaultdict(list)
        for result in results:
            by_file[result['file']].append(result)
        
        # Show up to 5 examples per pattern
        example_count = 0
        for filepath, file_results in sorted(by_file.items()):
            if example_count >= 5:
                remaining = len(by_file) - 5
                if remaining > 0:
                    print(f"  ... and {remaining} more files")
                break
            
            print(f"  {filepath}:")
            for result in file_results[:3]:  # Show up to 3 lines per file
                print(f"    Line {result['line']}: {result['content']}")
            if len(file_results) > 3:
                print(f"    ... and {len(file_results) - 3} more occurrences")
            example_count += 1
    
    # Print affected files summary
    print("\n" + "=" * 80)
    print(f"\nAFFECTED FILES SUMMARY:")
    print(f"Total files scanned: {total_files}")
    print(f"Files with Transaction usage: {len(affected_files)}")
    print(f"\nTotal occurrences: {sum(len(results) for results in all_results.values())}")
    
    # Save full results to file
    output_file = "transaction_usage_report.txt"
    with open(output_file, 'w') as f:
        f.write("FULL TRANSACTION USAGE REPORT\n")
        f.write("=" * 80 + "\n\n")
        
        for filepath in sorted(affected_files):
            f.write(f"\nFile: {filepath}\n")
            f.write("-" * 80 + "\n")
            
            file_results = []
            for results in all_results.values():
                file_results.extend([r for r in results if r['file'] == filepath])
            
            # Sort by line number
            file_results.sort(key=lambda x: x['line'])
            
            for result in file_results:
                f.write(f"Line {result['line']} [{result['pattern']}]: {result['content']}\n")
    
    print(f"\nFull report saved to: {output_file}")
    
    # Generate migration checklist
    print("\n" + "=" * 80)
    print("\nMIGRATION CHECKLIST:")
    print("-" * 80)
    
    critical_files = [
        'include/neo/ledger/block.h',
        'include/neo/ledger/memory_pool.h',
        'include/neo/network/p2p/payloads/transaction_payload.h',
        'src/ledger/blockchain.cpp',
        'src/network/p2p/transaction_router.cpp',
        'src/rpc/rpc_methods.cpp',
        'src/wallets/wallet_transaction.cpp',
    ]
    
    print("\nCritical files to update first:")
    for cf in critical_files:
        if any(cf in str(f) for f in affected_files):
            print(f"  ✓ {cf}")
        else:
            print(f"  - {cf} (check manually)")
    
    print("\nRecommended update order:")
    print("1. Update transaction.h to add Neo3Transaction alias")
    print("2. Update Block class to use Neo3Transaction")
    print("3. Update MemoryPool to use Neo3Transaction")
    print("4. Update network payloads")
    print("5. Update RPC methods")
    print("6. Update wallet code")
    print("7. Remove old Transaction, CoinReference, TransactionOutput")
    print("8. Update all tests")

if __name__ == "__main__":
    main()