# Li Moon - Release Process

This document describes how to create releases with pre-compiled binaries for the `install-binary.sh` script.

## Quick Release

```bash
# 1. Build the project
make clean && make

# 2. Create release archive
VERSION="0.1.0-alpha"
ARCH="amd64"  # or "arm64"
RELEASE_DIR="limoon-${VERSION}-linux-${ARCH}"

mkdir -p "releases/$RELEASE_DIR"

# Copy binary
cp li "releases/$RELEASE_DIR/"

# Copy runtime files
cp init.lua "releases/$RELEASE_DIR/"
cp -r core modules plugins themes docs "releases/$RELEASE_DIR/"

# Copy lexers (follow symlink)
cp -rL lexers "releases/$RELEASE_DIR/"

# Create tarball
cd releases
tar -czf "${RELEASE_DIR}.tar.gz" "$RELEASE_DIR"
cd ..

echo "Release created: releases/${RELEASE_DIR}.tar.gz"
```

## Automated Release Script

```bash
#!/bin/bash
# create-release.sh

VERSION="${1:-$(git describe --tags --always)}"
ARCH=$(uname -m)
case "$ARCH" in
  x86_64) ARCH_NAME="amd64" ;;
  aarch64) ARCH_NAME="arm64" ;;
  *) echo "Unknown arch: $ARCH"; exit 1 ;;
esac

# Build
make clean 2>/dev/null || true
make

# Check for lexers
if [ -L "lexers" ] && [ ! -e "lexers" ]; then
    echo "ERROR: lexers symlink is broken. Build may have failed."
    exit 1
fi

# Create release
RELEASE_NAME="limoon-${VERSION#v}-linux-${ARCH_NAME}"
mkdir -p "dist/$RELEASE_NAME"

cp li init.lua "dist/$RELEASE_NAME/"
cp -rL core modules plugins themes docs lexers "dist/$RELEASE_NAME/" 2>/dev/null || true

cd dist
tar -czf "${RELEASE_NAME}.tar.gz" "$RELEASE_NAME"
echo "Created: dist/${RELEASE_NAME}.tar.gz"
ls -lh "${RELEASE_NAME}.tar.gz"
```

## GitHub Release

1. Go to GitHub → Releases → "Create a new release"
2. Tag version: `v0.1.0-alpha`
3. Upload the tarball: `limoon-0.1.0-alpha-linux-amd64.tar.gz`
4. Publish release

The `install-binary.sh` will automatically detect and download the latest release.

## Testing Binary Installation

```bash
# Test local install
./install-binary.sh --local .

# Test remote install (after release published)
curl -sSf https://raw.githubusercontent.com/v0id-online/limoon/default/install-binary.sh | sh
```

## Troubleshooting

### "Failed to download binary"
- Check if the release exists on GitHub
- Verify the architecture matches (amd64/arm64)

### "notcurses library not found"
The binary requires notcurses to be installed system-wide:
- Fedora: `sudo dnf install notcurses`
- Ubuntu/Debian: `sudo apt install libnotcurses2`
- Arch: `sudo pacman -S notcurses`
