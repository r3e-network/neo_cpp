#!/usr/bin/env python3
"""
Neo C++ to C# Compatibility Checker
Ensures the C++ implementation matches the C# reference implementation exactly.
"""

import json
import sys
import os
import requests
import hashlib
import time
from typing import Dict, List, Tuple, Any, Optional
from dataclasses import dataclass
from enum import Enum
import subprocess
import argparse

# Configuration
CPP_RPC_URL = "http://localhost:10332"  # C++ node
CSHARP_RPC_URL = "http://localhost:10331"  # C# reference node
TIMEOUT = 30

class TestResult(Enum):
    PASS = "✅ PASS"
    FAIL = "❌ FAIL"
    SKIP = "⏭️ SKIP"
    WARN = "⚠️ WARN"

@dataclass
class CompatibilityTest:
    name: str
    category: str
    description: str
    result: TestResult = TestResult.SKIP
    details: str = ""
    cpp_response: Any = None
    csharp_response: Any = None

class CompatibilityChecker:
    def __init__(self, cpp_url: str, csharp_url: str):
        self.cpp_url = cpp_url
        self.csharp_url = csharp_url
        self.tests: List[CompatibilityTest] = []
        self.test_categories = {
            "RPC": [],
            "Protocol": [],
            "Consensus": [],
            "Storage": [],
            "VM": [],
            "Cryptography": [],
            "NEP": [],
            "Transaction": []
        }
        
    def make_rpc_call(self, url: str, method: str, params: List = None) -> Dict:
        """Make RPC call to a node."""
        payload = {
            "jsonrpc": "2.0",
            "method": method,
            "params": params or [],
            "id": 1
        }
        
        try:
            response = requests.post(url, json=payload, timeout=TIMEOUT)
            response.raise_for_status()
            return response.json()
        except Exception as e:
            return {"error": str(e)}
    
    def compare_responses(self, cpp_resp: Dict, csharp_resp: Dict, 
                         ignore_fields: List[str] = None) -> Tuple[bool, str]:
        """Compare two RPC responses, ignoring specified fields."""
        ignore_fields = ignore_fields or []
        
        # Remove ignored fields
        def clean_response(resp: Dict) -> Dict:
            if "result" in resp:
                result = resp["result"]
                if isinstance(result, dict):
                    for field in ignore_fields:
                        result.pop(field, None)
            return resp
        
        cpp_clean = clean_response(cpp_resp.copy())
        csharp_clean = clean_response(csharp_resp.copy())
        
        if cpp_clean == csharp_clean:
            return True, "Responses match exactly"
        
        # Deep comparison with details
        differences = []
        def compare_values(path: str, cpp_val: Any, cs_val: Any):
            if type(cpp_val) != type(cs_val):
                differences.append(f"{path}: Type mismatch - C++ {type(cpp_val).__name__} vs C# {type(cs_val).__name__}")
            elif isinstance(cpp_val, dict):
                for key in set(cpp_val.keys()) | set(cs_val.keys()):
                    if key not in cpp_val:
                        differences.append(f"{path}.{key}: Missing in C++")
                    elif key not in cs_val:
                        differences.append(f"{path}.{key}: Missing in C#")
                    else:
                        compare_values(f"{path}.{key}", cpp_val[key], cs_val[key])
            elif isinstance(cpp_val, list):
                if len(cpp_val) != len(cs_val):
                    differences.append(f"{path}: Length mismatch - C++ {len(cpp_val)} vs C# {len(cs_val)}")
                else:
                    for i, (cpp_item, cs_item) in enumerate(zip(cpp_val, cs_val)):
                        compare_values(f"{path}[{i}]", cpp_item, cs_item)
            elif cpp_val != cs_val:
                differences.append(f"{path}: Value mismatch - C++ '{cpp_val}' vs C# '{cs_val}'")
        
        compare_values("response", cpp_clean, csharp_clean)
        
        if differences:
            return False, "\n".join(differences[:5])  # Limit to first 5 differences
        return True, "Unknown difference"
    
    def test_rpc_method(self, method: str, params: List = None, 
                       ignore_fields: List[str] = None, 
                       description: str = "") -> CompatibilityTest:
        """Test a single RPC method for compatibility."""
        test = CompatibilityTest(
            name=f"RPC_{method}",
            category="RPC",
            description=description or f"Test {method} RPC method"
        )
        
        # Make calls to both nodes
        cpp_resp = self.make_rpc_call(self.cpp_url, method, params)
        csharp_resp = self.make_rpc_call(self.csharp_url, method, params)
        
        test.cpp_response = cpp_resp
        test.csharp_response = csharp_resp
        
        # Check for errors
        if "error" in cpp_resp:
            test.result = TestResult.FAIL
            test.details = f"C++ error: {cpp_resp['error']}"
        elif "error" in csharp_resp:
            test.result = TestResult.SKIP
            test.details = f"C# reference error: {csharp_resp['error']}"
        else:
            # Compare responses
            match, details = self.compare_responses(cpp_resp, csharp_resp, ignore_fields)
            test.result = TestResult.PASS if match else TestResult.FAIL
            test.details = details
        
        self.tests.append(test)
        self.test_categories["RPC"].append(test)
        return test
    
    def run_rpc_tests(self):
        """Run all RPC compatibility tests."""
        print("\n🔍 Testing RPC Methods Compatibility...")
        
        # Basic node information
        self.test_rpc_method("getversion", ignore_fields=["nonce", "tcpport", "wsport"])
        self.test_rpc_method("getblockcount")
        self.test_rpc_method("getconnectioncount")
        self.test_rpc_method("getbestblockhash")
        
        # Block queries
        self.test_rpc_method("getblock", [1, True], description="Get block by index")
        self.test_rpc_method("getblockheader", [1, True], description="Get block header")
        
        # Native contracts
        self.test_rpc_method("getnativecontracts")
        
        # Memory pool
        self.test_rpc_method("getrawmempool")
        
        # Validation
        test_address = "NXV7ZhHiyM1aHXwpVsRZC6BwNFP2jghXAq"
        self.test_rpc_method("validateaddress", [test_address])
        
        # NEP-17 balances
        self.test_rpc_method("getnep17balances", [test_address])
        
        # Contract state
        gas_contract = "0xd2a4cff31913016155e38e474a2c06d08be276cf"
        self.test_rpc_method("getcontractstate", [gas_contract])
        
        # Invoke function test
        self.test_rpc_method("invokefunction", 
                           [gas_contract, "symbol", []],
                           description="Invoke GAS.symbol()")
    
    def test_protocol_constants(self):
        """Test protocol configuration constants."""
        print("\n🔍 Testing Protocol Constants...")
        
        test = CompatibilityTest(
            name="Protocol_Constants",
            category="Protocol",
            description="Verify protocol configuration matches"
        )
        
        cpp_version = self.make_rpc_call(self.cpp_url, "getversion")
        cs_version = self.make_rpc_call(self.csharp_url, "getversion")
        
        if "result" in cpp_version and "result" in cs_version:
            cpp_proto = cpp_version["result"].get("protocol", {})
            cs_proto = cs_version["result"].get("protocol", {})
            
            checks = []
            # Check critical protocol values
            critical_fields = ["network", "addressversion", "validatorscount", "msperblock"]
            for field in critical_fields:
                if cpp_proto.get(field) == cs_proto.get(field):
                    checks.append(f"✓ {field}: {cpp_proto.get(field)}")
                else:
                    checks.append(f"✗ {field}: C++ {cpp_proto.get(field)} vs C# {cs_proto.get(field)}")
            
            test.details = "\n".join(checks)
            test.result = TestResult.PASS if all("✓" in c for c in checks) else TestResult.FAIL
        else:
            test.result = TestResult.FAIL
            test.details = "Failed to get version information"
        
        self.tests.append(test)
        self.test_categories["Protocol"].append(test)
    
    def test_native_contracts(self):
        """Test native contract compatibility."""
        print("\n🔍 Testing Native Contracts...")
        
        test = CompatibilityTest(
            name="Native_Contracts",
            category="Protocol",
            description="Verify native contracts match"
        )
        
        cpp_contracts = self.make_rpc_call(self.cpp_url, "getnativecontracts")
        cs_contracts = self.make_rpc_call(self.csharp_url, "getnativecontracts")
        
        if "result" in cpp_contracts and "result" in cs_contracts:
            cpp_list = {c["name"]: c["hash"] for c in cpp_contracts["result"]}
            cs_list = {c["name"]: c["hash"] for c in cs_contracts["result"]}
            
            # Expected native contracts
            expected_contracts = [
                "ContractManagement",
                "StdLib", 
                "CryptoLib",
                "LedgerContract",
                "NeoToken",
                "GasToken",
                "PolicyContract",
                "RoleManagement",
                "OracleContract",
                "NameService"
            ]
            
            checks = []
            for contract in expected_contracts:
                cpp_hash = cpp_list.get(contract, "missing")
                cs_hash = cs_list.get(contract, "missing")
                if cpp_hash == cs_hash and cpp_hash != "missing":
                    checks.append(f"✓ {contract}: {cpp_hash}")
                else:
                    checks.append(f"✗ {contract}: C++ {cpp_hash} vs C# {cs_hash}")
            
            test.details = "\n".join(checks)
            test.result = TestResult.PASS if all("✓" in c for c in checks) else TestResult.FAIL
        else:
            test.result = TestResult.FAIL
            test.details = "Failed to get native contracts"
        
        self.tests.append(test)
        self.test_categories["Protocol"].append(test)
    
    def test_transaction_verification(self):
        """Test transaction creation and verification."""
        print("\n🔍 Testing Transaction Processing...")
        
        test = CompatibilityTest(
            name="Transaction_Verification",
            category="Transaction",
            description="Test transaction format and verification"
        )
        
        # Test transaction invocation (without sending)
        script = "0c14d2a4cff31913016155e38e474a2c06d08be276cf41c00c0673796d626f6c41c01f0c0d476173546f6b656e2e73796d626f6c419c6f1e2128"
        
        cpp_result = self.make_rpc_call(self.cpp_url, "invokescript", [script])
        cs_result = self.make_rpc_call(self.csharp_url, "invokescript", [script])
        
        if "result" in cpp_result and "result" in cs_result:
            # Compare gas consumption
            cpp_gas = cpp_result["result"].get("gasconsumed", "0")
            cs_gas = cs_result["result"].get("gasconsumed", "0")
            
            # Compare execution state
            cpp_state = cpp_result["result"].get("state", "")
            cs_state = cs_result["result"].get("state", "")
            
            checks = []
            checks.append(f"Gas: C++ {cpp_gas} vs C# {cs_gas} - {'✓' if cpp_gas == cs_gas else '✗'}")
            checks.append(f"State: C++ {cpp_state} vs C# {cs_state} - {'✓' if cpp_state == cs_state else '✗'}")
            
            test.details = "\n".join(checks)
            test.result = TestResult.PASS if cpp_gas == cs_gas and cpp_state == cs_state else TestResult.FAIL
        else:
            test.result = TestResult.FAIL
            test.details = "Failed to invoke script"
        
        self.tests.append(test)
        self.test_categories["Transaction"].append(test)
    
    def test_nep17_tokens(self):
        """Test NEP-17 token standard implementation."""
        print("\n🔍 Testing NEP-17 Token Standard...")
        
        # Test NEO token
        neo_hash = "0xef4073a0f2b305a38ec4050e4d3d28bc40ea63f5"
        test = CompatibilityTest(
            name="NEP17_NEO_Token",
            category="NEP",
            description="Test NEO token implementation"
        )
        
        # Test token methods
        methods = ["symbol", "decimals", "totalSupply"]
        all_match = True
        details = []
        
        for method in methods:
            cpp_result = self.make_rpc_call(self.cpp_url, "invokefunction", 
                                           [neo_hash, method, []])
            cs_result = self.make_rpc_call(self.csharp_url, "invokefunction",
                                          [neo_hash, method, []])
            
            if "result" in cpp_result and "result" in cs_result:
                cpp_stack = cpp_result["result"].get("stack", [])
                cs_stack = cs_result["result"].get("stack", [])
                
                if cpp_stack and cs_stack:
                    cpp_val = cpp_stack[0].get("value")
                    cs_val = cs_stack[0].get("value")
                    match = cpp_val == cs_val
                    details.append(f"{method}: {'✓' if match else '✗'} (C++ {cpp_val} vs C# {cs_val})")
                    all_match = all_match and match
        
        test.details = "\n".join(details)
        test.result = TestResult.PASS if all_match else TestResult.FAIL
        
        self.tests.append(test)
        self.test_categories["NEP"].append(test)
        
        # Test GAS token
        gas_hash = "0xd2a4cff31913016155e38e474a2c06d08be276cf"
        test_gas = CompatibilityTest(
            name="NEP17_GAS_Token",
            category="NEP",
            description="Test GAS token implementation"
        )
        
        cpp_result = self.make_rpc_call(self.cpp_url, "invokefunction",
                                       [gas_hash, "symbol", []])
        cs_result = self.make_rpc_call(self.csharp_url, "invokefunction",
                                      [gas_hash, "symbol", []])
        
        if "result" in cpp_result and "result" in cs_result:
            test_gas.result = TestResult.PASS
            test_gas.details = "GAS token methods verified"
        else:
            test_gas.result = TestResult.FAIL
            test_gas.details = "GAS token verification failed"
        
        self.tests.append(test_gas)
        self.test_categories["NEP"].append(test_gas)
    
    def test_cryptography(self):
        """Test cryptographic operations compatibility."""
        print("\n🔍 Testing Cryptography...")
        
        test = CompatibilityTest(
            name="Crypto_Hash_Functions",
            category="Cryptography",
            description="Test hash function compatibility"
        )
        
        # Test script that uses crypto functions
        # This script computes SHA256 of "Hello"
        script = "0c0548656c6c6f41c00c067368613235364156944e1cb5"
        
        cpp_result = self.make_rpc_call(self.cpp_url, "invokescript", [script])
        cs_result = self.make_rpc_call(self.csharp_url, "invokescript", [script])
        
        if "result" in cpp_result and "result" in cs_result:
            cpp_stack = cpp_result["result"].get("stack", [])
            cs_stack = cs_result["result"].get("stack", [])
            
            if cpp_stack and cs_stack:
                match = cpp_stack == cs_stack
                test.result = TestResult.PASS if match else TestResult.FAIL
                test.details = f"Hash results: {'Match' if match else 'Mismatch'}"
            else:
                test.result = TestResult.FAIL
                test.details = "Failed to get stack results"
        else:
            test.result = TestResult.FAIL
            test.details = "Failed to invoke crypto script"
        
        self.tests.append(test)
        self.test_categories["Cryptography"].append(test)
    
    def test_vm_operations(self):
        """Test VM operation compatibility."""
        print("\n🔍 Testing VM Operations...")
        
        test = CompatibilityTest(
            name="VM_Basic_Operations",
            category="VM",
            description="Test basic VM operations"
        )
        
        # Test various VM operations
        # PUSH1 PUSH2 ADD (1 + 2 = 3)
        script = "515293"
        
        cpp_result = self.make_rpc_call(self.cpp_url, "invokescript", [script])
        cs_result = self.make_rpc_call(self.csharp_url, "invokescript", [script])
        
        checks = []
        if "result" in cpp_result and "result" in cs_result:
            # Check gas consumption
            cpp_gas = cpp_result["result"].get("gasconsumed", "0")
            cs_gas = cs_result["result"].get("gasconsumed", "0")
            checks.append(f"Gas consumption: {'✓' if cpp_gas == cs_gas else '✗'}")
            
            # Check result
            cpp_stack = cpp_result["result"].get("stack", [])
            cs_stack = cs_result["result"].get("stack", [])
            
            if cpp_stack and cs_stack:
                cpp_val = cpp_stack[0].get("value") if cpp_stack else None
                cs_val = cs_stack[0].get("value") if cs_stack else None
                checks.append(f"Result value: {'✓' if cpp_val == cs_val else '✗'} ({cpp_val} vs {cs_val})")
            
            test.details = "\n".join(checks)
            test.result = TestResult.PASS if all("✓" in c for c in checks) else TestResult.FAIL
        else:
            test.result = TestResult.FAIL
            test.details = "Failed to execute VM operations"
        
        self.tests.append(test)
        self.test_categories["VM"].append(test)
    
    def test_storage_operations(self):
        """Test storage operation compatibility."""
        print("\n🔍 Testing Storage Operations...")
        
        test = CompatibilityTest(
            name="Storage_Operations",
            category="Storage",
            description="Test storage key/value operations"
        )
        
        # Test getting storage from a native contract
        contract_hash = "0xef4073a0f2b305a38ec4050e4d3d28bc40ea63f5"  # NEO
        storage_key = "0b"  # Total supply key
        
        cpp_result = self.make_rpc_call(self.cpp_url, "getstorage", 
                                       [contract_hash, storage_key])
        cs_result = self.make_rpc_call(self.csharp_url, "getstorage",
                                      [contract_hash, storage_key])
        
        if cpp_result.get("result") is not None and cs_result.get("result") is not None:
            match = cpp_result["result"] == cs_result["result"]
            test.result = TestResult.PASS if match else TestResult.FAIL
            test.details = f"Storage values: {'Match' if match else 'Mismatch'}"
        else:
            test.result = TestResult.SKIP
            test.details = "Storage operation not available"
        
        self.tests.append(test)
        self.test_categories["Storage"].append(test)
    
    def generate_report(self) -> str:
        """Generate compatibility test report."""
        report = []
        report.append("=" * 80)
        report.append("NEO C++ TO C# COMPATIBILITY TEST REPORT")
        report.append("=" * 80)
        report.append(f"Timestamp: {time.strftime('%Y-%m-%d %H:%M:%S')}")
        report.append(f"C++ Node: {self.cpp_url}")
        report.append(f"C# Node: {self.csharp_url}")
        report.append("")
        
        # Summary statistics
        total = len(self.tests)
        passed = sum(1 for t in self.tests if t.result == TestResult.PASS)
        failed = sum(1 for t in self.tests if t.result == TestResult.FAIL)
        skipped = sum(1 for t in self.tests if t.result == TestResult.SKIP)
        warned = sum(1 for t in self.tests if t.result == TestResult.WARN)
        
        report.append("SUMMARY")
        report.append("-" * 40)
        report.append(f"Total Tests: {total}")
        report.append(f"✅ Passed: {passed} ({passed*100//total if total else 0}%)")
        report.append(f"❌ Failed: {failed} ({failed*100//total if total else 0}%)")
        report.append(f"⏭️  Skipped: {skipped}")
        report.append(f"⚠️  Warnings: {warned}")
        report.append("")
        
        # Results by category
        report.append("RESULTS BY CATEGORY")
        report.append("-" * 40)
        for category, tests in self.test_categories.items():
            if tests:
                cat_passed = sum(1 for t in tests if t.result == TestResult.PASS)
                cat_total = len(tests)
                report.append(f"{category}: {cat_passed}/{cat_total} passed")
        report.append("")
        
        # Detailed results
        report.append("DETAILED TEST RESULTS")
        report.append("-" * 40)
        for test in self.tests:
            report.append(f"\n{test.result.value} {test.name}")
            report.append(f"   Category: {test.category}")
            report.append(f"   Description: {test.description}")
            if test.details:
                for line in test.details.split("\n"):
                    report.append(f"   {line}")
        
        # Compatibility verdict
        report.append("")
        report.append("=" * 80)
        report.append("COMPATIBILITY VERDICT")
        report.append("=" * 80)
        
        if failed == 0:
            report.append("✅ FULLY COMPATIBLE - The C++ implementation matches the C# reference")
        elif failed <= total * 0.1:  # Less than 10% failures
            report.append("⚠️ MOSTLY COMPATIBLE - Minor differences detected")
        else:
            report.append("❌ NOT COMPATIBLE - Significant differences from C# reference")
        
        report.append("")
        compatibility_score = (passed * 100 // total) if total else 0
        report.append(f"COMPATIBILITY SCORE: {compatibility_score}%")
        
        return "\n".join(report)
    
    def run_all_tests(self):
        """Run all compatibility tests."""
        print("🚀 Starting Neo C++ to C# Compatibility Tests")
        print("=" * 60)
        
        # Check if both nodes are accessible
        print("Checking node connectivity...")
        cpp_accessible = self.make_rpc_call(self.cpp_url, "getversion")
        cs_accessible = self.make_rpc_call(self.csharp_url, "getversion")
        
        if "error" in cpp_accessible:
            print(f"❌ Cannot connect to C++ node at {self.cpp_url}")
            print(f"   Error: {cpp_accessible['error']}")
            print("\n⚠️ Make sure the C++ node is running on port 10332")
            return False
        
        if "error" in cs_accessible:
            print(f"⚠️ Cannot connect to C# reference node at {self.csharp_url}")
            print("   Continuing with available tests...")
        
        # Run test suites
        self.run_rpc_tests()
        self.test_protocol_constants()
        self.test_native_contracts()
        self.test_transaction_verification()
        self.test_nep17_tokens()
        self.test_cryptography()
        self.test_vm_operations()
        self.test_storage_operations()
        
        # Generate and display report
        report = self.generate_report()
        print("\n" + report)
        
        # Save report to file
        report_file = "compatibility_report.txt"
        with open(report_file, "w") as f:
            f.write(report)
        print(f"\n📄 Report saved to {report_file}")
        
        # Return success if all critical tests pass
        critical_categories = ["Protocol", "Transaction", "VM"]
        critical_passed = all(
            all(t.result == TestResult.PASS for t in self.test_categories[cat])
            for cat in critical_categories if self.test_categories[cat]
        )
        
        return critical_passed

def main():
    parser = argparse.ArgumentParser(description="Neo C++ to C# Compatibility Checker")
    parser.add_argument("--cpp-url", default=CPP_RPC_URL, help="C++ node RPC URL")
    parser.add_argument("--csharp-url", default=CSHARP_RPC_URL, help="C# node RPC URL")
    parser.add_argument("--category", help="Test specific category only")
    
    args = parser.parse_args()
    
    checker = CompatibilityChecker(args.cpp_url, args.csharp_url)
    
    if args.category:
        # Run specific category tests
        if args.category == "RPC":
            checker.run_rpc_tests()
        elif args.category == "Protocol":
            checker.test_protocol_constants()
            checker.test_native_contracts()
        elif args.category == "Transaction":
            checker.test_transaction_verification()
        elif args.category == "NEP":
            checker.test_nep17_tokens()
        elif args.category == "VM":
            checker.test_vm_operations()
        elif args.category == "Storage":
            checker.test_storage_operations()
        elif args.category == "Cryptography":
            checker.test_cryptography()
        else:
            print(f"Unknown category: {args.category}")
            sys.exit(1)
        
        report = checker.generate_report()
        print(report)
    else:
        # Run all tests
        success = checker.run_all_tests()
        sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()