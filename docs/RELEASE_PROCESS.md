# Neo C++ Release Process

## Overview
This document describes the release process for the Neo C++ node, including version management, binary building, and distribution.

## Release Types

### Major Release (X.0.0)
- Breaking changes
- Major feature additions
- Protocol upgrades
- Incompatible API changes

### Minor Release (0.X.0)
- New features
- Non-breaking improvements
- Performance enhancements
- New APIs (backward compatible)

### Patch Release (0.0.X)
- Bug fixes
- Security patches
- Documentation updates
- Minor improvements

## Release Workflow

### 1. Pre-Release Preparation

#### Code Freeze
- [ ] Stop accepting new features
- [ ] Focus on bug fixes and testing
- [ ] Update dependencies to stable versions

#### Testing
- [ ] Run full test suite locally
- [ ] Verify all CI checks pass
- [ ] Perform integration testing
- [ ] Test on all target platforms

#### Documentation
- [ ] Update README.md with new features
- [ ] Update API documentation
- [ ] Review and update examples
- [ ] Update configuration documentation

### 2. Version Bump

Use the version bump script:

```bash
# For patch release (0.1.0 -> 0.1.1)
./scripts/bump-version.sh patch

# For minor release (0.1.0 -> 0.2.0)
./scripts/bump-version.sh minor

# For major release (0.1.0 -> 1.0.0)
./scripts/bump-version.sh major

# For specific version
./scripts/bump-version.sh 1.2.3
```

### 3. Create Release

#### Automatic Release (Recommended)

1. **Push the tag to trigger workflow:**
   ```bash
   ./scripts/bump-version.sh minor --push
   ```

2. **Monitor GitHub Actions:**
   - Go to Actions tab on GitHub
   - Watch "Release" workflow progress
   - Verify all build jobs succeed

3. **Review draft release:**
   - Go to Releases page
   - Review auto-generated release notes
   - Edit and enhance as needed

#### Manual Release

1. **Create and push tag:**
   ```bash
   git tag -a v0.1.0 -m "Release v0.1.0"
   git push origin v0.1.0
   ```

2. **Trigger workflow manually:**
   - Go to Actions → Release workflow
   - Click "Run workflow"
   - Enter version number

### 4. Release Artifacts

The release workflow automatically builds and uploads:

#### Binary Packages
- **Linux (x64)**: `neo-node-linux-x64.tar.gz`
  - Neo node binary
  - CLI tool
  - Configuration files
  - Systemd service file
  - Start scripts

- **macOS (x64)**: `neo-node-macos-x64.tar.gz`
  - Neo node binary
  - CLI tool
  - Configuration files
  - LaunchAgent plist
  - Start scripts

- **Windows (x64)**: `neo-node-windows-x64.zip`
  - Neo node executable
  - CLI tool
  - Configuration files
  - Batch/PowerShell scripts
  - Required DLLs

#### Docker Images
- `ghcr.io/{owner}/neo-cpp:version`
- `ghcr.io/{owner}/neo-cpp:latest`
- Multi-arch support (amd64, arm64)

#### Checksums
- Individual SHA256 for each artifact
- Combined `checksums.txt` file

### 5. Post-Release

#### Publish Release
1. **Review draft release on GitHub**
2. **Add detailed release notes:**
   - New features
   - Bug fixes
   - Breaking changes
   - Migration guide
   - Known issues

3. **Publish the release**

#### Announcements
- [ ] Update project website
- [ ] Post on social media
- [ ] Notify community channels
- [ ] Update documentation site

#### Verification
- [ ] Download and test binaries
- [ ] Verify Docker images work
- [ ] Check documentation links
- [ ] Test installation instructions

## Release Checklist

### Before Release
- [ ] All tests passing
- [ ] Documentation updated
- [ ] Changelog prepared
- [ ] Version bumped
- [ ] Dependencies updated
- [ ] Security scan clean

### During Release
- [ ] Tag created and pushed
- [ ] GitHub Actions workflow triggered
- [ ] All builds successful
- [ ] Artifacts uploaded
- [ ] Docker images pushed

### After Release
- [ ] Release published on GitHub
- [ ] Release notes complete
- [ ] Community notified
- [ ] Documentation deployed
- [ ] Binaries verified

## Troubleshooting

### Build Failures

#### Linux Build Issues
- Check dependency versions
- Verify CMake configuration
- Review compiler output

#### macOS Build Issues
- Check Xcode/CommandLineTools version
- Verify Homebrew packages
- Review signing requirements

#### Windows Build Issues
- Check MSVC version
- Verify vcpkg packages
- Review PATH configuration

### Workflow Issues

#### Authentication Errors
- Verify GITHUB_TOKEN permissions
- Check repository settings
- Review workflow permissions

#### Upload Failures
- Check artifact size limits
- Verify network connectivity
- Review GitHub API limits

## Emergency Procedures

### Rollback Release
1. Delete the release on GitHub
2. Delete the tag:
   ```bash
   git push --delete origin v0.1.0
   git tag -d v0.1.0
   ```
3. Fix issues
4. Create new release

### Hotfix Process
1. Create hotfix branch from tag
2. Apply fix and test
3. Bump patch version
4. Create new release
5. Merge fix to main branch

## Automation Scripts

### Version Management
```bash
# Show current version
cat VERSION

# Bump with dry run
./scripts/bump-version.sh patch --dry-run

# Bump and push
./scripts/bump-version.sh minor --push
```

### Local Testing
```bash
# Build release binary locally
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

# Test the binary
./build/apps/neo_node --version
```

### Release Validation
```bash
# Download and verify release
wget https://github.com/{owner}/neo-cpp/releases/download/v0.1.0/neo-node-linux-x64.tar.gz
wget https://github.com/{owner}/neo-cpp/releases/download/v0.1.0/neo-node-linux-x64.tar.gz.sha256

# Verify checksum
sha256sum -c neo-node-linux-x64.tar.gz.sha256

# Extract and test
tar -xzf neo-node-linux-x64.tar.gz
cd neo-node-linux-x64
./bin/neo-node --version
```

## Security Considerations

### Code Signing
- Sign binaries on macOS/Windows (when certificates available)
- Provide GPG signatures for Linux packages
- Include checksums for all artifacts

### Vulnerability Scanning
- Run security scans before release
- Check dependencies for CVEs
- Review security advisories

### Access Control
- Limit release permissions
- Use protected branches
- Require PR reviews

## Support

For release issues:
1. Check GitHub Actions logs
2. Review this documentation
3. Contact the development team
4. File an issue on GitHub

---

*Last Updated: August 2025*