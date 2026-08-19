#!/bin/sh
# Li Moon — binary installer (no compilation required)
#
# Two modes, auto-detected:
#   - Bundled: run from inside an extracted release tarball
#     (limoon-<version>-<os>-<arch>.tar.gz), where this script sits
#     alongside `li`, `init.lua`, `core/`, etc. — installs those files
#     directly, no network needed.
#   - Standalone: sh <(curl -sSf https://raw.githubusercontent.com/v0id-online/limoon/default/install-binary.sh)
#     — downloads the latest (or $LIMOON_VERSION) release tarball first.
set -e

REPO_URL="https://github.com/v0id-online/limoon"
INSTALL_DIR="${LIMOON_INSTALL_DIR:-$HOME/.local/share/limoon}"
BIN_DIR="${LIMOON_BIN_DIR:-$HOME/.local/bin}"
BINARY="$INSTALL_DIR/li"
WRAPPER="$BIN_DIR/li"
SRC_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"

# ── helpers (defined first — every other function may call these) ───────────

info()  { printf '\033[1;34m==> \033[0m%s\n' "$*"; }
ok()    { printf '\033[1;32m ok \033[0m%s\n' "$*"; }
die()   { printf '\033[1;31mERR \033[0m%s\n' "$*" >&2; exit 1; }

need() {
  command -v "$1" >/dev/null 2>&1 || die "Required tool not found: $1. Please install it and retry."
}

# ── architecture ──────────────────────────────────────────────────────────────

detect_arch() {
  ARCH=$(uname -m)
  case "$ARCH" in
    x86_64)  ARCH_NAME="x86_64" ;;
    aarch64) ARCH_NAME="aarch64" ;;
    *) die "Unsupported architecture: $ARCH" ;;
  esac
}

# ── download (standalone mode only) ──────────────────────────────────────────

get_latest_release() {
  info "Detecting latest release..."
  LATEST=""
  if command -v curl >/dev/null 2>&1; then
    LATEST=$(curl -sSf "https://api.github.com/repos/v0id-online/limoon/releases/latest" 2>/dev/null |
             grep '"tag_name":' | sed -E 's/.*"([^"]+)".*/\1/')
  elif command -v wget >/dev/null 2>&1; then
    LATEST=$(wget -qO- "https://api.github.com/repos/v0id-online/limoon/releases/latest" 2>/dev/null |
             grep '"tag_name":' | sed -E 's/.*"([^"]+)".*/\1/')
  fi
  [ -n "$LATEST" ] || die "Could not detect the latest release. Set LIMOON_VERSION=vX.Y.Z and retry, or use the bundled installer from a downloaded release tarball instead."
  ok "Latest version: $LATEST"
}

download_binary() {
  version="$1"
  filename="limoon-${version#v}-linux-${ARCH_NAME}.tar.gz"
  url="$REPO_URL/releases/download/$version/$filename"
  tmpdir=$(mktemp -d)

  info "Downloading $filename ..."
  if command -v curl >/dev/null 2>&1; then
    curl -sSfL "$url" -o "$tmpdir/limoon.tar.gz" || { rm -rf "$tmpdir"; die "Failed to download binary from $url"; }
  else
    wget -q "$url" -O "$tmpdir/limoon.tar.gz" || { rm -rf "$tmpdir"; die "Failed to download binary from $url"; }
  fi
  ok "Download complete."

  info "Extracting..."
  tar -xzf "$tmpdir/limoon.tar.gz" -C "$tmpdir" --strip-components=1 || { rm -rf "$tmpdir"; die "Failed to extract archive"; }

  DOWNLOADED_DIR="$tmpdir"
}

# ── install ───────────────────────────────────────────────────────────────────

install_files() {
  payload_dir="$1"
  [ -f "$payload_dir/li" ] || die "li binary not found in $payload_dir — incomplete payload."
  [ -f "$payload_dir/init.lua" ] || die "init.lua not found in $payload_dir — incomplete payload."

  info "Installing to $INSTALL_DIR ..."
  mkdir -p "$INSTALL_DIR" "$BIN_DIR"

  cp "$payload_dir/li" "$BINARY"
  chmod 755 "$BINARY"

  cp "$payload_dir/init.lua" "$INSTALL_DIR/"
  for d in core modules plugins themes docs lexers; do
    if [ -d "$payload_dir/$d" ]; then
      cp -r "$payload_dir/$d" "$INSTALL_DIR/"
    fi
  done
  if [ -f "$payload_dir/LICENSE" ]; then
    cp "$payload_dir/LICENSE" "$INSTALL_DIR/"
  fi

  ok "Files installed."

  # Wrapper script so user can just type 'li'.
  # Important: the binary needs LIMOON_HOME pointing to the install directory.
  info "Creating wrapper script at $WRAPPER ..."
  cat > "$WRAPPER" <<EOF
#!/bin/sh
export LIMOON_HOME="$INSTALL_DIR"
exec "$BINARY" "\$@"
EOF
  chmod 755 "$WRAPPER"
  ok "Wrapper created."
}

# ── runtime dependency check (informational only — nothing to build) ─────────

check_runtime_deps() {
  if ! ldd "$BINARY" 2>/dev/null | grep -q "not found"; then
    ok "All shared library dependencies resolved."
  else
    printf '\033[1;33m  !\033[0m Some shared libraries are missing:\n'
    ldd "$BINARY" 2>/dev/null | grep "not found" | sed 's/^/      /'
    printf '      Install notcurses (and ncurses) via your package manager and retry.\n'
  fi
}

# ── path hint ─────────────────────────────────────────────────────────────────

path_hint() {
  case ":${PATH}:" in
    *":$BIN_DIR:"*) return ;;
  esac
  printf '\n\033[1;33mNOTE:\033[0m %s is not in your PATH.\n' "$BIN_DIR"
  printf 'Add the following line to your shell profile (~/.bashrc, ~/.zshrc, etc.):\n\n'
  printf '  export PATH="%s:$PATH"\n\n' "$BIN_DIR"
}

# ── main ─────────────────────────────────────────────────────────────────────

main() {
  printf '\n'
  printf '  Li Moon — Binary Installer\n'
  printf '  ===========================\n\n'

  need mkdir; need chmod; need cp

  DOWNLOADED_DIR=""
  if [ -f "$SRC_DIR/li" ] && [ -f "$SRC_DIR/init.lua" ]; then
    info "Bundled payload found next to this script — installing locally, no download needed."
    install_files "$SRC_DIR"
  else
    need tar
    command -v curl >/dev/null 2>&1 || command -v wget >/dev/null 2>&1 ||
      die "Neither curl nor wget found. Install one of them, or run this script from inside an extracted release tarball."
    detect_arch
    VERSION="${LIMOON_VERSION:-}"
    if [ -z "$VERSION" ]; then get_latest_release; VERSION="$LATEST"; fi
    download_binary "$VERSION"
    install_files "$DOWNLOADED_DIR"
  fi
  [ -n "$DOWNLOADED_DIR" ] && rm -rf "$DOWNLOADED_DIR"

  check_runtime_deps

  printf '\n'
  printf '\033[1;32m  Installation complete!\033[0m\n'
  printf '  Run: li\n\n'

  if command -v xsel >/dev/null 2>&1 || command -v wl-copy >/dev/null 2>&1; then
    printf '\033[1;32m  ✓\033[0m System clipboard integration available\n'
  else
    printf '\033[1;33m  !\033[0m For system clipboard support, install: xsel (X11) or wl-clipboard (Wayland)\n'
  fi

  printf '\n  Available themes: dark, light, gruvbox-dark, gruvbox-light,\n'
  printf '                    neon-cyberpunk, neon-purple, neon-blue,\n'
  printf '                    neon-amber, matrix, vaporwave, and 22 more!\n'
  printf '  Set theme: type Ctrl+P, "Select Theme" (or limoon.themes.set(\"neon-cyberpunk\") in ~/.limoon/init.lua)\n\n'

  path_hint
}

main "$@"
