#!/bin/sh
# Li Moon — Binary install script (no compilation required)
# Usage: sh <(curl -sSf https://raw.githubusercontent.com/v0id-online/limoon/default/install-binary.sh)
set -e

REPO_URL="https://github.com/v0id-online/limoon"
INSTALL_DIR="${LIMOON_INSTALL_DIR:-$HOME/.local/share/limoon}"
BIN_DIR="${LIMOON_BIN_DIR:-$HOME/.local/bin}"
WRAPPER="$BIN_DIR/li"

# Detect architecture
ARCH=$(uname -m)
case "$ARCH" in
  x86_64)  ARCH_NAME="amd64" ;;
  aarch64) ARCH_NAME="arm64" ;;
  *) die "Unsupported architecture: $ARCH" ;;
esac

# ── helpers ──────────────────────────────────────────────────────────────────

info()  { printf '\033[1;34m==> \033[0m%s\n' "$*"; }
ok()    { printf '\033[1;32m ok \033[0m%s\n' "$*"; }
die()   { printf '\033[1;31mERR \033[0m%s\n' "$*" >&2; exit 1; }

need() {
  command -v "$1" >/dev/null 2>&1 || die "Required tool not found: $1. Please install it and retry."
}

# ── detect latest release ────────────────────────────────────────────────────

get_latest_release() {
  info "Detecting latest release..."
  
  # Try using curl with GitHub API
  if command -v curl >/dev/null 2>&1; then
    LATEST=$(curl -sSf "https://api.github.com/repos/v0id-online/limoon/releases/latest" 2>/dev/null | 
             grep '"tag_name":' | 
             sed -E 's/.*"([^"]+)".*/\1/')
  fi
  
  # Fallback to default
  if [ -z "$LATEST" ]; then
    LATEST="v0.1.0-alpha"
    info "Could not detect latest version, using $LATEST"
  else
    ok "Latest version: $LATEST"
  fi
  
  echo "$LATEST"
}

# ── download binary ──────────────────────────────────────────────────────────

download_binary() {
  local version="$1"
  local filename="limoon-${version#v}-linux-${ARCH_NAME}.tar.gz"
  local url="$REPO_URL/releases/download/$version/$filename"
  local tmpdir=$(mktemp -d)
  
  info "Downloading $filename ..."
  
  if command -v curl >/dev/null 2>&1; then
    curl -sSfL "$url" -o "$tmpdir/limoon.tar.gz" || {
      rm -rf "$tmpdir"
      die "Failed to download binary from $url"
    }
  elif command -v wget >/dev/null 2>&1; then
    wget -q "$url" -O "$tmpdir/limoon.tar.gz" || {
      rm -rf "$tmpdir"
      die "Failed to download binary from $url"
    }
  else
    die "Neither curl nor wget found. Please install one of them."
  fi
  
  ok "Download complete."
  
  # Extract
  info "Extracting..."
  tar -xzf "$tmpdir/limoon.tar.gz" -C "$tmpdir" || {
    rm -rf "$tmpdir"
    die "Failed to extract archive"
  }
  
  echo "$tmpdir"
}

# ── install ─────────────────────────────────────────────────────────────────

install_files() {
  local src_dir="$1"
  
  info "Installing to $INSTALL_DIR ..."
  
  # Create directories
  mkdir -p "$INSTALL_DIR" "$BIN_DIR"
  
  # Remove old installation if exists
  if [ -d "$INSTALL_DIR" ] && [ -f "$INSTALL_DIR/li" ]; then
    info "Removing old installation..."
    rm -rf "$INSTALL_DIR"
    mkdir -p "$INSTALL_DIR"
  fi
  
  # Install binary
  if [ -f "$src_dir/li" ]; then
    cp "$src_dir/li" "$INSTALL_DIR/"
  elif [ -f "$src_dir/bin/li" ]; then
    cp "$src_dir/bin/li" "$INSTALL_DIR/"
  else
    # Find binary
    local found_bin=$(find "$src_dir" -name "li" -type f -executable 2>/dev/null | head -1)
    if [ -n "$found_bin" ]; then
      cp "$found_bin" "$INSTALL_DIR/"
    else
      die "Could not find 'li' binary in archive"
    fi
  fi
  chmod 755 "$INSTALL_DIR/li"
  ok "Binary installed."
  
  # Install runtime files
  for item in init.lua core modules plugins themes docs lexers; do
    if [ -e "$src_dir/$item" ]; then
      cp -r "$src_dir/$item" "$INSTALL_DIR/"
    fi
  done
  ok "Runtime files installed."
  
  # Create wrapper script
  info "Creating wrapper script at $WRAPPER ..."
  cat > "$WRAPPER" <<EOF
#!/bin/sh
# Li Moon wrapper script
export LIMOON_HOME="$INSTALL_DIR"
exec "$INSTALL_DIR/li" "\$@"
EOF
  chmod 755 "$WRAPPER"
  ok "Wrapper created."
}

# ── check dependencies ───────────────────────────────────────────────────────

check_deps() {
  info "Checking dependencies..."
  
  # Check for notcurses
  if ! ldconfig -p 2>/dev/null | grep -q notcurses; then
    if [ -f /etc/os-release ]; then
      . /etc/os-release
      case "$ID $ID_LIKE" in
        *fedora*|*rhel*|*centos*)
          info "notcurses library not found. Install with: sudo dnf install notcurses"
          ;;
        *debian*|*ubuntu*)
          info "notcurses library not found. Install with: sudo apt install libnotcurses2"
          ;;
        *arch*|*manjaro*)
          info "notcurses library not found. Install with: sudo pacman -S notcurses"
          ;;
        *)
          info "notcurses library not found. Please install it manually."
          ;;
      esac
    fi
  fi
  
  ok "Dependency check complete."
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

# ── local install fallback ───────────────────────────────────────────────────

install_from_local() {
  local src_dir="${1:-.}"
  
  info "Installing from local directory: $src_dir"
  
  if [ ! -f "$src_dir/li" ]; then
    die "Binary not found in $src_dir. Please build first with: make"
  fi
  
  install_files "$src_dir"
}

# ── main ─────────────────────────────────────────────────────────────────────

main() {
  printf '\n'
  printf '  Li Moon — Binary Installer\n'
  printf '  ==========================\n\n'
  
  need mkdir
  need chmod
  need cp
  need tar
  
  # Check for local install mode
  if [ "${LOCAL_INSTALL:-0}" = "1" ]; then
    install_from_local "${LOCAL_DIR:-.}"
  else
    # Online install - need curl or wget
    if ! command -v curl >/dev/null 2>&1 && ! command -v wget >/dev/null 2>&1; then
      die "Neither curl nor wget found. Install one of them or use LOCAL_INSTALL=1"
    fi
    
    VERSION=$(get_latest_release)
    TMPDIR=$(download_binary "$VERSION")
    
    install_files "$TMPDIR"
    
    # Cleanup
    rm -rf "$TMPDIR"
  fi
  
  check_deps
  
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
  printf '                    neon-amber, matrix, vaporwave, and 23 more!\n'
  printf '  Set theme: view:set_theme(\"neon-cyberpunk\") in ~/.limoon/init.lua\n\n'
  
  path_hint
}

# Handle command line
if [ "${1:-}" = "--local" ] || [ "${1:-}" = "-l" ]; then
  LOCAL_INSTALL=1
  LOCAL_DIR="${2:-.}"
fi

main "$@"
