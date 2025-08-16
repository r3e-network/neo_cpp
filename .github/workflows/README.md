# GitHub Actions Workflows

## Active Workflows

### 1. `ci.yml` - Main CI/CD Pipeline
**Purpose**: Primary continuous integration for all commits
**Triggers**: 
- Push to main/master/develop branches
- Pull requests
- Manual dispatch

**Features**:
- Multi-platform builds (Ubuntu latest, Ubuntu 22.04, macOS)
- Parallel test execution
- Code quality checks (formatting, static analysis)
- Build caching for faster runs
- 30-minute timeout

**Jobs**:
- `build-and-test`: Compile and run tests on all platforms
- `code-quality`: Static analysis and formatting checks
- `build-status`: Summary of build results

### 2. `release.yml` - Release Management
**Purpose**: Create official releases with binaries
**Triggers**:
- Push tags starting with 'v' (e.g., v1.0.0)
- Manual dispatch with version input

**Features**:
- Multi-platform release builds (Linux, macOS)
- Automatic GitHub release creation
- Binary artifacts packaging
- Release notes generation

**Jobs**:
- `build-release`: Build binaries for each platform
- `create-release`: Create GitHub release with artifacts

## Archived Workflows

The following workflows have been archived to `archived/` directory to reduce clutter:
- `build-test.yml` - Redundant with main CI
- `ci-cd-optimized.yml` - Merged into main CI
- `ci-full-tests.yml` - Integrated into main CI
- `ci-simple.yml` - Replaced by main CI
- `test.yml` - Duplicate functionality
- `quality-gates.yml` - Integrated into main CI
- `release-build.yml` - Replaced by simplified release.yml

## Usage

### Running CI
CI runs automatically on:
- Every push to main/master/develop
- Every pull request

Manual trigger:
```bash
gh workflow run ci.yml
```

### Creating a Release
1. **Via Git tag**:
```bash
git tag v1.0.0
git push origin v1.0.0
```

2. **Via GitHub CLI**:
```bash
gh workflow run release.yml -f version=v1.0.0
```

3. **Via GitHub UI**:
- Go to Actions tab
- Select "Release" workflow
- Click "Run workflow"
- Enter version (e.g., v1.0.0)

### Monitoring Workflows

**List recent runs**:
```bash
gh run list --limit 10
```

**View specific run**:
```bash
gh run view [RUN_ID]
```

**View logs**:
```bash
gh run view [RUN_ID] --log
```

**Cancel stuck workflow**:
```bash
gh run cancel [RUN_ID]
```

## Best Practices

1. **Keep it simple**: Two main workflows cover all needs
2. **Fast feedback**: CI provides quick validation
3. **Reliable releases**: Automated release process
4. **Resource efficiency**: Build caching and parallel jobs
5. **Timeout protection**: 30-45 minute limits prevent stuck jobs

## Troubleshooting

### Build Failures
- Check logs for specific error messages
- Ensure all dependencies are correctly specified
- Verify CMake configuration

### Test Failures
- Tests run with `continue-on-error` to complete all tests
- Check uploaded test artifacts for details
- Run tests locally to reproduce issues

### Release Issues
- Ensure tag follows `v*` pattern (e.g., v1.0.0)
- Verify GitHub token permissions
- Check artifact upload succeeded

## Configuration

All workflows use:
- **Latest action versions** (v4/v5)
- **Concurrency control** to cancel old runs
- **Timeout limits** to prevent hanging
- **Matrix builds** for cross-platform support
- **Caching** for faster builds

---
*Last Updated: August 2024*