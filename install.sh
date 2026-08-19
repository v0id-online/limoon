#!/bin/sh
# Li Moon — install script
# Usage:  sh <(curl -sSf https://raw.githubusercontent.com/v0id-online/limoon/default/install.sh)
set -e

REPO_URL="https://github.com/v0id-online/limoon.git"
INSTALL_DIR="${LIMOON_INSTALL_DIR:-$HOME/.local/share/limoon}"
BIN_DIR="${LIMOON_BIN_DIR:-$HOME/.local/bin}"
BINARY="$INSTALL_DIR/li"
WRAPPER="$BIN_DIR/li"
WORK_DIR="${LIMOON_BUILD_DIR:-/tmp/limoon-build-$$}"

# ── helpers ──────────────────────────────────────────────────────────────────

info()  { printf '\033[1;34m==> \033[0m%s\n' "$*"; }
ok()    { printf '\033[1;32m ok \033[0m%s\n' "$*"; }
die()   { printf '\033[1;31mERR \033[0m%s\n' "$*" >&2; exit 1; }

need() {
  command -v "$1" >/dev/null 2>&1 || die "Required tool not found: $1. Please install it and retry."
}

# ── detect distro & install deps ─────────────────────────────────────────────

install_deps() {
  info "Detecting Linux distribution..."

  if [ -f /etc/os-release ]; then
    # shellcheck disable=SC1091
    . /etc/os-release
    DISTRO="${ID:-unknown}"
    DISTRO_LIKE="${ID_LIKE:-}"
  else
    die "Cannot detect distribution (/etc/os-release not found)."
  fi

  # Normalise: if DISTRO_LIKE contains a known family, use it
  case "$DISTRO $DISTRO_LIKE" in
    *fedora*|*rhel*|*centos*)  FAMILY=rpm  ;;
    *debian*|*ubuntu*)         FAMILY=deb  ;;
    *arch*|*manjaro*)          FAMILY=arch ;;
    *) die "Unsupported distro: $DISTRO. Install deps manually and re-run with SKIP_DEPS=1." ;;
  esac

  info "Distro family: $FAMILY — installing build dependencies..."

  case "$FAMILY" in
    rpm)
      PKGS="git cmake gcc gcc-c++ make pkgconf-pkg-config ncurses-devel notcurses-devel"
      if command -v dnf >/dev/null 2>&1; then
        sudo dnf install -y $PKGS
      else
        sudo yum install -y $PKGS
      fi
      # Optional: clipboard integration (install if available, but don't fail if not)
      sudo dnf install -y xsel 2>/dev/null || true
      ;;
    deb)
      sudo apt-get update -qq
      sudo apt-get install -y git cmake gcc g++ make pkg-config libncurses-dev libnotcurses-dev
      # Optional: clipboard integration (try wl-clipboard first for Wayland, then xsel for X11)
      sudo apt-get install -y wl-clipboard 2>/dev/null || sudo apt-get install -y xsel 2>/dev/null || true
      ;;
    arch)
      sudo pacman -Sy --noconfirm git cmake gcc make pkgconf ncurses notcurses
      # Optional: clipboard integration
      sudo pacman -S --noconfirm wl-clipboard xsel 2>/dev/null || true
      ;;
  esac

  ok "Dependencies installed."
}

# ── clone ────────────────────────────────────────────────────────────────────

clone_repo() {
  info "Cloning repository into $WORK_DIR ..."
  git clone --recurse-submodules --depth=1 "$REPO_URL" "$WORK_DIR"
  ok "Clone complete."
}

# ── build ────────────────────────────────────────────────────────────────────

build() {
  info "Building Lua/LPeg/LFS/regex static libraries (cmake)..."
  cmake -S "$WORK_DIR" -B "$WORK_DIR/build" \
    -DQT=OFF -DGTK3=OFF -DGTK2=OFF \
    -DFETCHCONTENT_QUIET=OFF \
    -DCMAKE_BUILD_TYPE=Release \
    > "$WORK_DIR/cmake-deps.log" 2>&1 || {
      cat "$WORK_DIR/cmake-deps.log"
      die "cmake configure failed — see log above."
    }

  NPROC=$(nproc 2>/dev/null || echo 4)
  cmake --build "$WORK_DIR/build" --target lua lpeg lfs regex \
    --parallel "$NPROC" \
    >> "$WORK_DIR/cmake-deps.log" 2>&1 || {
      cat "$WORK_DIR/cmake-deps.log"
      die "cmake build of deps failed — see log above."
    }
  ok "Static libraries built."

  info "Building limoon (make)..."
  make -C "$WORK_DIR" -j"$NPROC" 2>&1 | tee "$WORK_DIR/make.log" || {
    die "make failed — see $WORK_DIR/make.log"
  }
  ok "Binary built: $WORK_DIR/li"
}

# ── install ───────────────────────────────────────────────────────────────────

install_files() {
  info "Installing to $INSTALL_DIR ..."
  mkdir -p "$INSTALL_DIR" "$BIN_DIR"

  # Binary
  cp "$WORK_DIR/li" "$BINARY"
  chmod 755 "$BINARY"

  # Lua runtime files (same dir as binary so _HOME resolves correctly)
  cp "$WORK_DIR/init.lua" "$INSTALL_DIR/"
  for d in core modules plugins themes docs; do
    if [ -d "$WORK_DIR/$d" ]; then
      cp -r "$WORK_DIR/$d" "$INSTALL_DIR/"
    fi
  done

  # Lexers (if present)
  if [ -d "$WORK_DIR/lexers" ]; then
    # If it's a symbolic link, copy the actual content
    if [ -L "$WORK_DIR/lexers" ]; then
      cp -rL "$WORK_DIR/lexers" "$INSTALL_DIR/"
    else
      cp -r "$WORK_DIR/lexers" "$INSTALL_DIR/"
    fi
  fi

  ok "Files installed."

  # Wrapper script so user can just type 'li'
  # Important: the binary needs LIMOON_HOME pointing to the install directory
  info "Creating wrapper script at $WRAPPER ..."
  cat > "$WRAPPER" <<EOF
#!/bin/sh
export LIMOON_HOME="$INSTALL_DIR"
exec "$BINARY" "\$@"
EOF
  chmod 755 "$WRAPPER"
  ok "Wrapper created."
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

# ── cleanup ───────────────────────────────────────────────────────────────────

cleanup() {
  if [ "${KEEP_BUILD:-0}" = "1" ]; then
    info "Build directory kept at $WORK_DIR (KEEP_BUILD=1)"
  else
    info "Removing build directory $WORK_DIR ..."
    rm -rf "$WORK_DIR"
  fi
}

# ── main ─────────────────────────────────────────────────────────────────────

main() {
  printf '\n'
  printf '  Li Moon — Installer\n'
  printf '  ====================\n\n'

  if [ "${SKIP_DEPS:-0}" != "1" ]; then
    install_deps
  else
    info "SKIP_DEPS=1 — skipping dependency installation."
  fi

  need git
  need cmake
  need make
  need gcc
  need pkg-config

  clone_repo
  build
  install_files
  cleanup

  printf '\n'
  printf '\033[1;32m  Installation complete!\033[0m\n'
  printf '  Run: li\n\n'
  
  # Optional features info
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
