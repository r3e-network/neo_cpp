# GitHub Actions Workflows

## Active Workflows

### 1. `release-ultra-simple.yml` 
**Purpose**: Create releases with binaries for all platforms
**Trigger**: Push tags `v*.*.*` or manual
**Status**: Primary release mechanism

### 2. `build-test.yml`
**Purpose**: Basic CI build and test on push/PR
**Trigger**: Push to master, Pull requests
**Status**: Active for CI

### 3. `quality-gates-lite.yml`
**Purpose**: Quick quality checks
**Trigger**: Push to master
**Status**: Active, consistently passing

## Archived Workflows

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