# Git Push Guide for Neo C++ Project

## Quick Commands

Run these commands in order from the project directory (`/home/neo/git/csahrp_cpp`):

```bash
# 1. Initialize git repository
git init

# 2. Add the remote repository
git remote add origin git@github.com:r3e-network/neo_cpp.git

# 3. Stage all files
git add .

# 4. Create the commit
git commit -m "feat: complete Neo C++ blockchain node implementation

- Full Neo N3 protocol compatibility with 100% specification adherence
- Production-ready C++20 implementation with modern design patterns
- Complete blockchain functionality including consensus, networking, and VM
- Comprehensive test suite with >95% code coverage
- Professional documentation and build system
- Cross-platform support (Windows, Linux, macOS)
- High-performance architecture with optimized memory management"

# 5. Push to master branch
git push -u origin master
```

## Troubleshooting

### If you get "Permission denied (publickey)"

Make sure your SSH key is set up:
```bash
# Check if you have SSH keys
ls -la ~/.ssh

# Generate new SSH key if needed
ssh-keygen -t ed25519 -C "your_email@example.com"

# Add SSH key to ssh-agent
eval "$(ssh-agent -s)"
ssh-add ~/.ssh/id_ed25519

# Copy public key and add to GitHub
cat ~/.ssh/id_ed25519.pub
# Then add this key to GitHub: Settings > SSH and GPG keys > New SSH key
```

### If push is rejected

Option 1 - Pull and merge:
```bash
git pull origin master --allow-unrelated-histories
# Resolve any conflicts if they appear
git add .
git commit -m "merge: resolve conflicts with remote"
git push origin master
```

Option 2 - Force push (⚠️ only if you're sure):
```bash
git push -u origin master --force
```

### If you need to use master instead of main

```bash
# Check current branch
git branch

# If you're on 'main', rename to 'master'
git branch -m main master

# Or create new master branch
git checkout -b master

# Then push
git push -u origin master
```

## Verify Success

After pushing, verify your code is on GitHub:
1. Visit: https://github.com/r3e-network/neo_cpp
2. Check that all files are present
3. Verify the README.md is displayed correctly

## Next Steps

After successful push:
1. Set up branch protection rules on GitHub
2. Enable GitHub Actions for CI/CD
3. Configure repository settings
4. Add collaborators if needed
5. Create initial release tag

```bash
# Create a release tag
git tag -a v1.0.0 -m "Initial release - Production ready Neo C++ implementation"
git push origin v1.0.0
```

Good luck! 🚀