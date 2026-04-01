#!/bin/bash
# Create a Li Moon release tarball

set -e

VERSION="${1:-$(git describe --tags --always 2>/dev/null || echo 'dev')}"
ARCH=$(uname -m)
case "$ARCH" in
  x86_64) ARCH_NAME="amd64" ;;
  aarch64) ARCH_NAME="arm64" ;;
  *) echo "Unknown architecture: $ARCH"; exit 1 ;;
esac

RELEASE_NAME="limoon-${VERSION#v}-linux-${ARCH_NAME}"

info()  { printf '\033[1;34m==>\033[0m %s\n' "$*"; }
ok()    { printf '\033[1;32m ok\033[0m %s\n' "$*"; }
die()   { printf '\033[1;31mERR\033[0m %s\n' "$*" >&2; exit 1; }

info "Creating release: $RELEASE_NAME"

# Build
info "Building..."
make clean 2>/dev/null || true
make -j$(nproc)

# Check binary exists
if [ ! -f "li" ]; then
    die "Binary 'li' not found after build"
fi

# Setup lexers if needed
if [ ! -e "lexers" ]; then
    if [ -d "build/_deps/scintillua-src/lexers" ]; then
        ln -s "build/_deps/scintillua-src/lexers" lexers
    fi
fi

# Check for broken lexers link
if [ -L "lexers" ] && [ ! -e "lexers" ]; then
    die "lexers symlink is broken. Build may have failed."
fi

# Create release directory
mkdir -p "dist"
rm -rf "dist/$RELEASE_NAME"
mkdir -p "dist/$RELEASE_NAME"

# Copy files
info "Copying files..."
cp li init.lua "dist/$RELEASE_NAME/"

for dir in core modules plugins themes docs; do
    if [ -d "$dir" ]; then
        cp -r "$dir" "dist/$RELEASE_NAME/"
    fi
done

# Copy lexers (follow symlink if needed)
if [ -d "lexers" ]; then
    if [ -L "lexers" ]; then
        cp -rL lexers "dist/$RELEASE_NAME/"
    else
        cp -r lexers "dist/$RELEASE_NAME/"
    fi
fi

# Create tarball
info "Creating tarball..."
cd dist
tar -czf "${RELEASE_NAME}.tar.gz" "$RELEASE_NAME"
cd ..

# Cleanup
rm -rf "dist/$RELEASE_NAME"

ok "Release created: dist/${RELEASE_NAME}.tar.gz"
ls -lh "dist/${RELEASE_NAME}.tar.gz"

echo ""
echo "To upload to GitHub:"
echo "  gh release create $VERSION dist/${RELEASE_NAME}.tar.gz --title \"$VERSION\" --notes \"Release $VERSION\""
