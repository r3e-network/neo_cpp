#\!/usr/bin/env python3
import xml.etree.ElementTree as ET
import glob
import json
from datetime import datetime

# Parse XML test results
def parse_xml_results(xml_file):
    try:
        tree = ET.parse(xml_file)
        root = tree.getroot()
        
        return {
            'tests': int(root.get('tests', 0)),
            'failures': int(root.get('failures', 0)),
            'disabled': int(root.get('disabled', 0)),
            'errors': int(root.get('errors', 0)),
            'time': float(root.get('time', 0)),
            'timestamp': root.get('timestamp', ''),
            'name': root.get('name', '')
        }
    except:
        return None

# Collect all results
results = []
for xml_file in glob.glob('*_test_results.xml'):
    result = parse_xml_results(xml_file)
    if result:
        results.append(result)

# Calculate totals
total_tests = sum(r['tests'] for r in results)
total_failures = sum(r['failures'] for r in results)
total_time = sum(r['time'] for r in results)

# Calculate pass rate
pass_rate = ((total_tests - total_failures) / total_tests * 100) if total_tests > 0 else 0

# Output metrics
print(f"Total Tests: {total_tests}")
print(f"Passed: {total_tests - total_failures}")
print(f"Failed: {total_failures}")
print(f"Pass Rate: {pass_rate:.1f}%")
print(f"Total Time: {total_time:.3f}s")
print(f"Avg Time/Test: {(total_time/total_tests*1000):.2f}ms" if total_tests > 0 else "N/A")
