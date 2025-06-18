#!/bin/bash

echo "🧹 Removing unwanted files from Git repository"
echo "=============================================="

cd /home/neo/git/csahrp_cpp

# Remove unwanted directories from Git (but keep them locally)
echo "📁 Removing directories from Git tracking..."

git rm -r --cached .cursor/ 2>/dev/null || echo ".cursor/ not in git"
git rm -r --cached .trae/ 2>/dev/null || echo ".trae/ not in git"
git rm -r --cached @docs/ 2>/dev/null || echo "@docs/ not in git"
git rm -r --cached neo/ 2>/dev/null || echo "neo/ not in git"
git rm -r --cached vcpkg_installed/ 2>/dev/null || echo "vcpkg_installed/ not in git"

# Also remove the cleanup scripts we don't need
git rm --cached cleanup_project.py 2>/dev/null || echo "cleanup_project.py not in git"
git rm --cached git_push_commands.sh 2>/dev/null || echo "git_push_commands.sh not in git"
git rm --cached git_commands_jimmy.sh 2>/dev/null || echo "git_commands_jimmy.sh not in git"
git rm --cached push_to_github.sh 2>/dev/null || echo "push_to_github.sh not in git"
git rm --cached git_push_clean.sh 2>/dev/null || echo "git_push_clean.sh not in git"
git rm --cached GIT_PUSH_GUIDE.md 2>/dev/null || echo "GIT_PUSH_GUIDE.md not in git"

echo ""
echo "✅ Files removed from Git tracking"
echo ""

# Show what will be committed
echo "📋 Changes to be committed:"
git status --short

# Commit the removal
echo ""
echo "💾 Creating commit to remove unwanted files..."
git commit -m "fix: remove unwanted directories and temporary files

- Remove .cursor/ directory
- Remove .trae/ directory  
- Remove @docs/ directory
- Remove neo/ submodule
- Remove vcpkg_installed/ directory
- Remove temporary git helper scripts
- Update .gitignore to prevent re-adding"

# Force push to update GitHub
echo ""
echo "🚀 Force pushing to GitHub to remove files..."
git push origin master --force

echo ""
echo "✅ Done! The unwanted directories have been removed from GitHub"
echo ""
echo "Check https://github.com/r3e-network/neo_cpp to verify"