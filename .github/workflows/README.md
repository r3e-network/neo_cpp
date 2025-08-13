# GitHub Actions Workflows

## Overview
All workflows are configured with:
- **30-minute timeout** to prevent stuck jobs
- **Concurrency controls** to cancel old runs when new commits are pushed  
- **Latest action versions** (v4) for optimal performance
- **Cross-platform support** for Linux and macOS

## Active Workflows

### 1. `ci.yml` - Continuous Integration
**Purpose**: Build and test on multiple platforms
**Trigger**: Push/PR to main, master, develop
**Matrix**: Ubuntu (latest, 22.04), Debug/Release builds
**Features**: ccache, parallel builds, test execution

### 2. `test.yml` - Test Suite  
**Purpose**: Comprehensive test execution with coverage
**Trigger**: Push/PR to main branches, manual dispatch
**Coverage**: Unit tests, integration tests, coverage reports
**Upload**: Test results and coverage to Codecov

### 3. `build-test.yml` - Build Validation
**Purpose**: Basic build and test validation
**Trigger**: Push/PR to main branches
**Platform**: Ubuntu latest
**Quick**: Fast feedback for commits

### 4. `release.yml` - Multi-Platform Release
**Purpose**: Create releases with binaries for all platforms
**Trigger**: Release branches, version tags, manual
**Platforms**: Linux and macOS
**Artifacts**: Packaged binaries with documentation

### 5. `release-build.yml` - Release Builder
**Purpose**: Simplified release build process
**Trigger**: Release branches, tags, manual
**Output**: Linux x64 binaries
**Features**: Automatic GitHub release creation

### 6. `ci-cd-optimized.yml` - Optimized Pipeline
**Purpose**: Fast CI/CD with smart caching
**Trigger**: Push/PR to main branches, tags
**Features**: Quick validation, parallel jobs, build caching

### 7. `quality-gates.yml` - Quality Checks
**Purpose**: Code quality and security validation  
**Trigger**: Push/PR, weekly schedule, manual
**Checks**: Linting, formatting, security scanning

## Configuration

### `release.yml` (Original)
- Complex but feature-complete
- Has dependency issues
- Keep for reference only

### `release-simplified.yml` 
- Intermediate solution
- Partially working
- Backup option if ultra-simple fails

## Disabled Workflows

The following workflows have been disabled (renamed to `.disabled`):
- `build-test-simple.yml.disabled` - Redundant
- `build-test-fixed.yml.disabled` - Not needed
- `c-cpp.yml.disabled` - Duplicate CI
- `validate-all.yml.disabled` - Infrastructure issues
- `quality-gates.yml.disabled` - Too complex
- `security.yml.disabled` - Not configured
- `ci.yml.disabled` - Duplicate

## Quick Commands

### Create a Release
```bash
# Option 1: Use workflow
gh workflow run release-ultra-simple.yml -f tag=v1.0.0

# Option 2: Local build
./scripts/local-release.sh v1.0.0
```

### Check Workflow Status
```bash
# List recent runs
gh run list --limit=10

# View specific workflow
gh run view [RUN_ID]

# Cancel stuck workflow
gh run cancel [RUN_ID]
```

### Monitor Releases
```bash
# List releases
gh release list

# View release
gh release view v1.0.0
```

## Troubleshooting

### If workflows are stuck in queue:
1. Cancel the run: `gh run cancel [RUN_ID]`
2. Use local build instead: `./scripts/local-release.sh`
3. Manually upload: `gh release upload v1.0.0 *.tar.gz`

### If builds fail:
1. Check logs: `gh run view [RUN_ID] --log`
2. Use minimal dependencies in CMake
3. Try local build as fallback

## Best Practices

1. **Keep it simple**: Fewer workflows = fewer problems
2. **Local fallback**: Always have a local build option
3. **Monitor queues**: Cancel stuck jobs after 10 minutes
4. **Use caching**: Speed up builds when possible
5. **Document everything**: Keep this README updated

---
*Last Updated: August 13, 2025*