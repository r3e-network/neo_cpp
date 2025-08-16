# Neo C++ Test Dashboard

A comprehensive test monitoring and reporting dashboard for the Neo C++ blockchain implementation.

## Features

- **Real-time Test Monitoring**: Live updates of test execution status
- **Code Coverage Analysis**: Module-level coverage reporting with visual progress bars
- **Performance Benchmarks**: Track performance metrics over time
- **Build History**: Historical view of test runs and build status
- **Interactive Filtering**: Filter tests by status (passed, failed, skipped)
- **Responsive Design**: Works on desktop and mobile devices

## Quick Start

### Automated Setup

Run the all-in-one script to build, test, and launch the dashboard:

```bash
cd tests/dashboard
chmod +x run_tests_dashboard.sh
./run_tests_dashboard.sh
```

This will:
1. Build the project with coverage enabled
2. Run all tests with XML output
3. Generate code coverage reports
4. Run performance benchmarks
5. Generate dashboard data
6. Start a local web server

Access the dashboard at: http://localhost:8080

### Manual Setup

#### 1. Build with Coverage

```bash
cd build
cmake .. -DENABLE_COVERAGE=ON -DNEO_BUILD_TESTS=ON
make -j$(nproc)
```

#### 2. Run Tests

```bash
# Run tests with XML output
./tests/unit/test_runner --gtest_output="xml:test_results.xml"

# Or run with console output
./tests/unit/test_runner | tee test_output.txt
```

#### 3. Generate Coverage Report

```bash
# Capture coverage data
lcov --capture --directory build --output-file coverage.info

# Remove external files
lcov --remove coverage.info '/usr/*' '*/tests/*' --output-file coverage.info

# Generate HTML report
genhtml coverage.info --output-directory coverage_html
```

#### 4. Run Benchmarks

```bash
./tests/performance/benchmark_crypto \
    --benchmark_format=json \
    --benchmark_out=benchmarks.json
```

#### 5. Generate Dashboard Data

```bash
cd tests/dashboard
python3 generate_results.py test_results.xml coverage.info benchmarks.json
```

#### 6. View Dashboard

Open `tests/dashboard/index.html` in a web browser, or serve it locally:

```bash
# Python 3
python3 -m http.server 8080

# Python 2
python -m SimpleHTTPServer 8080
```

## Dashboard Components

### Summary Statistics
- **Total Tests**: Number of test cases
- **Pass Rate**: Percentage of passing tests
- **Code Coverage**: Overall code coverage percentage
- **Test Duration**: Total execution time
- **Skipped Tests**: Number of disabled/skipped tests
- **Build Status**: Current build status

### Test Categories
Visual breakdown of tests by category:
- Unit Tests
- SDK Tests
- Integration Tests
- Performance Tests

### Coverage by Module
Module-level code coverage with progress bars:
- Core
- Cryptography
- Network
- SDK

### Test Results Table
Detailed test results by suite:
- Test count per suite
- Pass/fail/skip counts
- Execution time
- Status badges

### Performance Benchmarks
Key performance metrics:
- SHA256 throughput
- AES encryption speed
- Block processing rate
- Transaction verification speed

### Build History
Recent build results with timestamps and status indicators.

## Data Files

### Input Files
- `test_output.txt`: Console output from test run
- `test_results.xml`: Google Test XML output
- `coverage.info`: lcov coverage data
- `benchmarks.json`: Google Benchmark JSON output

### Output Files
- `test-results.json`: Combined test data for dashboard
- `test-results-history.json`: Historical test data
- `index.html`: Dashboard interface

## Requirements

### Build Requirements
- CMake 3.20+
- C++20 compiler
- Google Test
- Google Benchmark (optional)

### Dashboard Requirements
- Python 3.6+ (for data generation)
- Modern web browser (Chrome, Firefox, Safari, Edge)
- lcov/gcov (for coverage reports)

## Customization

### Modify Dashboard Appearance

Edit `index.html` to customize:
- Color scheme (CSS variables)
- Chart types and layouts
- Data refresh intervals
- Filter options

### Add Custom Metrics

Extend `generate_results.py` to parse additional data:
```python
def parse_custom_metrics(self, metrics_file):
    # Add custom parsing logic
    pass
```

### Integration with CI/CD

The dashboard can be integrated with CI/CD pipelines:

```yaml
# GitHub Actions example
- name: Run Tests with Dashboard
  run: |
    ./tests/dashboard/run_tests_dashboard.sh
    
- name: Upload Dashboard
  uses: actions/upload-artifact@v2
  with:
    name: test-dashboard
    path: tests/dashboard/
```

## Troubleshooting

### Tests Not Found
- Ensure tests are built: `make test_runner`
- Check test executable paths in the script

### Coverage Not Working
- Install lcov: `apt-get install lcov` or `brew install lcov`
- Ensure `-DENABLE_COVERAGE=ON` is set during CMake configuration

### Dashboard Not Loading
- Check that `test-results.json` exists
- Verify Python is installed for the web server
- Try opening `index.html` directly in a browser

### Performance Issues
- Reduce test history size (default: 100 entries)
- Disable auto-refresh in the dashboard
- Use a production web server for better performance

## Future Enhancements

- [ ] WebSocket support for real-time updates
- [ ] Database backend for historical data
- [ ] Test failure analysis with ML
- [ ] Integration with code review tools
- [ ] Mobile app for test monitoring
- [ ] Slack/Discord notifications
- [ ] Distributed test execution support
- [ ] Test flakiness detection

## Contributing

To add new features to the dashboard:

1. Fork the repository
2. Create a feature branch
3. Add tests for new functionality
4. Update documentation
5. Submit a pull request

## License

Same as Neo C++ project license.