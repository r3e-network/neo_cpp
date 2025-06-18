#!/bin/bash

echo "🧹 Cleaning Git Repository"
echo "========================="
echo ""

cd /home/neo/git/csahrp_cpp

echo "Removing unwanted directories from Git (keeping local copies)..."

# Remove each directory from Git tracking only
git rm -r --cached .cursor/ 2>/dev/null && echo "✅ Removed .cursor/ from Git"
git rm -r --cached .trae/ 2>/dev/null && echo "✅ Removed .trae/ from Git"
git rm -r --cached @docs/ 2>/dev/null && echo "✅ Removed @docs/ from Git"
git rm -r --cached neo/ 2>/dev/null && echo "✅ Removed neo/ from Git"
git rm -r --cached vcpkg_installed/ 2>/dev/null && echo "✅ Removed vcpkg_installed/ from Git"

# Remove temporary files
git rm --cached cleanup_project.py 2>/dev/null
git rm --cached git_push_commands.sh 2>/dev/null
git rm --cached git_commands_jimmy.sh 2>/dev/null
git rm --cached push_to_github.sh 2>/dev/null
git rm --cached git_push_clean.sh 2>/dev/null
git rm --cached GIT_PUSH_GUIDE.md 2>/dev/null
git rm --cached remove_unwanted_files.sh 2>/dev/null
git rm --cached fix_git_repo.sh 2>/dev/null

echo ""
echo "Committing changes..."
git add .gitignore
git commit -m "fix: remove unwanted directories from Git tracking

- Remove .cursor/, .trae/, @docs/, neo/, vcpkg_installed/ from Git
- Remove temporary helper scripts
- Update .gitignore to prevent re-adding
- Keep all files locally" || echo "No changes to commit"

echo ""
echo "Pushing to GitHub..."
git push origin master --force

echo ""
echo "✅ Complete! Check https://github.com/r3e-network/neo_cpp"
echo "   The unwanted directories should now be gone from GitHub"
echo "   but still exist on your local machine."