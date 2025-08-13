#!/bin/bash

# bump-version.sh - Version management script for Neo C++
# Helps create new releases by updating version numbers and creating tags

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Current version file locations
VERSION_FILE="VERSION"
CMAKE_FILE="CMakeLists.txt"

# Function to display usage
usage() {
    echo "Usage: $0 [major|minor|patch|<version>]"
    echo ""
    echo "Arguments:"
    echo "  major     - Bump major version (1.0.0 -> 2.0.0)"
    echo "  minor     - Bump minor version (1.0.0 -> 1.1.0)"
    echo "  patch     - Bump patch version (1.0.0 -> 1.0.1)"
    echo "  <version> - Set specific version (e.g., 1.2.3)"
    echo ""
    echo "Options:"
    echo "  --dry-run - Show what would be done without making changes"
    echo "  --no-tag  - Update version without creating git tag"
    echo "  --push    - Push tag to remote after creation"
    echo ""
    echo "Examples:"
    echo "  $0 patch                 # Bump patch version"
    echo "  $0 minor --push          # Bump minor and push"
    echo "  $0 1.5.0                 # Set version to 1.5.0"
    echo "  $0 major --dry-run       # Preview major bump"
    exit 1
}

# Parse arguments
BUMP_TYPE=""
NEW_VERSION=""
DRY_RUN=false
NO_TAG=false
PUSH_TAG=false

while [[ $# -gt 0 ]]; do
    case $1 in
        major|minor|patch)
            BUMP_TYPE=$1
            shift
            ;;
        --dry-run)
            DRY_RUN=true
            shift
            ;;
        --no-tag)
            NO_TAG=true
            shift
            ;;
        --push)
            PUSH_TAG=true
            shift
            ;;
        --help|-h)
            usage
            ;;
        *)
            # Check if it's a version number
            if [[ $1 =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
                NEW_VERSION=$1
            else
                echo -e "${RED}Error: Invalid argument '$1'${NC}"
                usage
            fi
            shift
            ;;
    esac
done

# Check if we have either bump type or new version
if [ -z "$BUMP_TYPE" ] && [ -z "$NEW_VERSION" ]; then
    echo -e "${RED}Error: Must specify either bump type or version${NC}"
    usage
fi

# Get current version
get_current_version() {
    if [ -f "$VERSION_FILE" ]; then
        cat "$VERSION_FILE" | tr -d '\n'
    else
        # Try to get from git tags
        git describe --tags --abbrev=0 2>/dev/null | sed 's/^v//' || echo "0.0.0"
    fi
}

# Bump version based on type
bump_version() {
    local current=$1
    local type=$2
    
    # Parse current version
    IFS='.' read -r major minor patch <<< "$current"
    
    case $type in
        major)
            major=$((major + 1))
            minor=0
            patch=0
            ;;
        minor)
            minor=$((minor + 1))
            patch=0
            ;;
        patch)
            patch=$((patch + 1))
            ;;
    esac
    
    echo "${major}.${minor}.${patch}"
}

# Update version in files
update_version_files() {
    local version=$1
    
    # Update VERSION file
    echo "$version" > "$VERSION_FILE"
    echo -e "${GREEN}✓${NC} Updated $VERSION_FILE"
    
    # Update CMakeLists.txt if it exists
    if [ -f "$CMAKE_FILE" ]; then
        # Update project version
        sed -i.bak "s/VERSION [0-9]\+\.[0-9]\+\.[0-9]\+/VERSION ${version}/" "$CMAKE_FILE"
        rm "${CMAKE_FILE}.bak"
        echo -e "${GREEN}✓${NC} Updated $CMAKE_FILE"
    fi
    
    # Update package.json if it exists (for any JS tooling)
    if [ -f "package.json" ]; then
        sed -i.bak "s/\"version\": \"[0-9]\+\.[0-9]\+\.[0-9]\+\"/\"version\": \"${version}\"/" package.json
        rm "package.json.bak"
        echo -e "${GREEN}✓${NC} Updated package.json"
    fi
}

# Main execution
echo "======================================"
echo "   NEO C++ VERSION MANAGEMENT"
echo "======================================"
echo ""

# Get current version
CURRENT_VERSION=$(get_current_version)
echo -e "Current version: ${BLUE}${CURRENT_VERSION}${NC}"

# Determine new version
if [ -n "$NEW_VERSION" ]; then
    TARGET_VERSION="$NEW_VERSION"
else
    TARGET_VERSION=$(bump_version "$CURRENT_VERSION" "$BUMP_TYPE")
fi

echo -e "New version:     ${GREEN}${TARGET_VERSION}${NC}"
echo ""

# Check if this is a dry run
if [ "$DRY_RUN" = true ]; then
    echo -e "${YELLOW}DRY RUN MODE - No changes will be made${NC}"
    echo ""
    echo "Would perform the following actions:"
    echo "1. Update version to ${TARGET_VERSION} in:"
    echo "   - $VERSION_FILE"
    [ -f "$CMAKE_FILE" ] && echo "   - $CMAKE_FILE"
    [ -f "package.json" ] && echo "   - package.json"
    
    if [ "$NO_TAG" = false ]; then
        echo "2. Create git tag: v${TARGET_VERSION}"
        echo "3. Commit changes with message: 'chore: bump version to ${TARGET_VERSION}'"
        
        if [ "$PUSH_TAG" = true ]; then
            echo "4. Push tag to remote repository"
        fi
    fi
    
    exit 0
fi

# Confirm with user
echo "This will update the version from ${CURRENT_VERSION} to ${TARGET_VERSION}"
if [ "$NO_TAG" = false ]; then
    echo "and create git tag v${TARGET_VERSION}"
fi
echo ""
read -p "Continue? (y/N) " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo "Aborted."
    exit 0
fi

echo ""

# Update version files
update_version_files "$TARGET_VERSION"

# Handle git operations
if [ "$NO_TAG" = false ]; then
    # Check for uncommitted changes
    if ! git diff-index --quiet HEAD --; then
        echo ""
        echo "Committing version changes..."
        git add "$VERSION_FILE"
        [ -f "$CMAKE_FILE" ] && git add "$CMAKE_FILE"
        [ -f "package.json" ] && git add "package.json"
        
        git commit -m "chore: bump version to ${TARGET_VERSION}

- Updated version from ${CURRENT_VERSION} to ${TARGET_VERSION}
- Prepared for release v${TARGET_VERSION}"
        
        echo -e "${GREEN}✓${NC} Created commit"
    fi
    
    # Create tag
    echo "Creating git tag..."
    TAG_NAME="v${TARGET_VERSION}"
    
    # Create annotated tag with release notes template
    TAG_MESSAGE="Release ${TARGET_VERSION}

## What's Changed
- Version bump from ${CURRENT_VERSION} to ${TARGET_VERSION}

## Release Notes
[Add release notes here]

## Installation
See README.md for installation instructions.
"
    
    git tag -a "$TAG_NAME" -m "$TAG_MESSAGE"
    echo -e "${GREEN}✓${NC} Created tag: $TAG_NAME"
    
    # Push if requested
    if [ "$PUSH_TAG" = true ]; then
        echo ""
        echo "Pushing to remote..."
        git push origin main
        git push origin "$TAG_NAME"
        echo -e "${GREEN}✓${NC} Pushed tag to remote"
        
        echo ""
        echo -e "${GREEN}Release workflow will be triggered automatically!${NC}"
        echo "Check GitHub Actions for build progress."
    else
        echo ""
        echo "Tag created locally. To push to remote, run:"
        echo -e "${BLUE}  git push origin main${NC}"
        echo -e "${BLUE}  git push origin ${TAG_NAME}${NC}"
    fi
else
    echo ""
    echo -e "${YELLOW}Version updated but no git tag created (--no-tag specified)${NC}"
fi

echo ""
echo "======================================"
echo -e "${GREEN}✅ Version bump complete!${NC}"
echo "======================================"
echo ""
echo "Next steps:"
if [ "$NO_TAG" = false ] && [ "$PUSH_TAG" = false ]; then
    echo "1. Push the tag to trigger release workflow:"
    echo "   git push origin v${TARGET_VERSION}"
fi
echo "2. Monitor GitHub Actions for release build"
echo "3. Once built, edit the release notes on GitHub"
echo "4. Publish the release"