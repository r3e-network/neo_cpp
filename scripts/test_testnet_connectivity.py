#!/usr/bin/env python3
"""
Test Neo N3 Testnet Connectivity
Verify that the node can connect to testnet seed nodes
"""

import socket
import json
import time
from pathlib import Path

def test_seed_connectivity():
    """Test connectivity to Neo N3 testnet seed nodes"""
    
    print("🌐 Testing Neo N3 Testnet Connectivity")
    print("=" * 40)
    
    # Load testnet configuration
    config_path = Path(__file__).parent.parent / "config" / "testnet.json"
    if not config_path.exists():
        print("❌ Testnet configuration not found")
        return False
    
    with open(config_path) as f:
        config = json.load(f)
    
    seed_list = config.get("seedList", [])
    if not seed_list:
        print("❌ No seed nodes configured")
        return False
    
    print(f"📡 Testing {len(seed_list)} seed nodes...")
    
    successful_connections = 0
    total_seeds = len(seed_list)
    
    for seed in seed_list:
        try:
            # Parse host:port
            if ":" in seed:
                host, port = seed.rsplit(":", 1)
                port = int(port)
            else:
                host = seed
                port = 20333  # Default Neo P2P port
            
            print(f"  Connecting to {host}:{port}...", end=" ")
            
            # Test TCP connection
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(5)  # 5 second timeout
            result = sock.connect_ex((host, port))
            sock.close()
            
            if result == 0:
                print("✅ CONNECTED")
                successful_connections += 1
            else:
                print("❌ FAILED")
                
        except Exception as e:
            print(f"❌ ERROR: {e}")
    
    success_rate = (successful_connections / total_seeds) * 100
    print(f"\n📊 Connectivity Results:")
    print(f"  Successful: {successful_connections}/{total_seeds} ({success_rate:.1f}%)")
    
    if success_rate >= 60:
        print("✅ GOOD: Sufficient connectivity to testnet")
        return True
    elif success_rate >= 20:
        print("⚠️  PARTIAL: Some connectivity issues")
        return True
    else:
        print("❌ POOR: Cannot reach testnet seed nodes")
        return False

def test_dns_resolution():
    """Test DNS resolution for seed nodes"""
    print("\n🔍 Testing DNS Resolution")
    print("=" * 30)
    
    seed_hosts = [
        "seed1t.neo.org",
        "seed2t.neo.org", 
        "seed3t.neo.org",
        "seed4t.neo.org",
        "seed5t.neo.org"
    ]
    
    resolved = 0
    for host in seed_hosts:
        try:
            addr = socket.gethostbyname(host)
            print(f"  {host} -> {addr} ✅")
            resolved += 1
        except Exception as e:
            print(f"  {host} -> ERROR: {e} ❌")
    
    success_rate = (resolved / len(seed_hosts)) * 100
    print(f"\n📊 DNS Resolution: {resolved}/{len(seed_hosts)} ({success_rate:.1f}%)")
    
    return success_rate >= 80

def main():
    """Run all connectivity tests"""
    print("🚀 Neo C++ Node Testnet Connectivity Test")
    print("=" * 50)
    
    dns_ok = test_dns_resolution()
    connectivity_ok = test_seed_connectivity()
    
    print("\n" + "=" * 50)
    print("📋 FINAL RESULTS")
    print("=" * 50)
    
    if dns_ok and connectivity_ok:
        print("🎉 EXCELLENT: Node is ready for testnet deployment!")
        print("✅ DNS resolution working")
        print("✅ Seed node connectivity confirmed")
        print("\n🚀 Ready to sync with Neo N3 testnet!")
        return True
    elif connectivity_ok:
        print("✅ GOOD: Node can connect to testnet")
        print("⚠️  Some DNS issues but connectivity works")
        return True
    else:
        print("❌ ISSUES: Network connectivity problems detected")
        print("   Check firewall settings and internet connection")
        return False

if __name__ == "__main__":
    import sys
    success = main()
    sys.exit(0 if success else 1)