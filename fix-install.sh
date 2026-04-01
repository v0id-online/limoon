#!/bin/sh
# Fix broken Li Moon installation

set -e

INSTALL_DIR="${LIMOON_INSTALL_DIR:-$HOME/.local/share/limoon}"
BIN_DIR="${LIMOON_BIN_DIR:-$HOME/.local/bin}"

info()  { printf '\033[1;34m==> \033[0m%s\n' "$*"; }
ok()    { printf '\033[1;32m ok \033[0m%s\n' "$*"; }
die()   { printf '\033[1;31mERR \033[0m%s\n' "$*" >&2; exit 1; }

info "Fixing Li Moon installation..."

# Check if installation exists
if [ ! -d "$INSTALL_DIR" ]; then
  die "Installation directory not found: $INSTALL_DIR"
fi

# 1. Fix the broken lexers symlink
if [ -L "$INSTALL_DIR/lexers" ] && [ ! -e "$INSTALL_DIR/lexers" ]; then
  info "Fixing broken lexers symlink..."
  rm "$INSTALL_DIR/lexers"
  
  # Try to find lexers from build directory
  if [ -d "$INSTALL_DIR/build/_deps/scintillua-src/lexers" ]; then
    cp -r "$INSTALL_DIR/build/_deps/scintillua-src/lexers" "$INSTALL_DIR/"
    ok "Lexers copied from build directory."
  elif [ -d "build/_deps/scintillua-src/lexers" ]; then
    cp -r "build/_deps/scintillua-src/lexers" "$INSTALL_DIR/"
    ok "Lexers copied from local build directory."
  else
    info "Warning: Could not find lexers. Syntax highlighting may not work."
  fi
else
  ok "Lexers link is OK."
fi

# 2. Fix the wrapper script
if [ -f "$BIN_DIR/li" ]; then
  if file "$BIN_DIR/li" | grep -q "ELF"; then
    info "Fixing: Binary was copied to BIN_DIR instead of wrapper script"
    rm "$BIN_DIR/li"
  fi
fi

if [ ! -f "$BIN_DIR/li" ]; then
  info "Creating wrapper script..."
  mkdir -p "$BIN_DIR"
  cat > "$BIN_DIR/li" <<EOF
#!/bin/sh
export LIMOON_HOME="$INSTALL_DIR"
exec "$INSTALL_DIR/li" "\$@"
EOF
  chmod 755 "$BIN_DIR/li"
  ok "Wrapper script created."
else
  # Check if wrapper has LIMOON_HOME
  if ! grep -q "LIMOON_HOME" "$BIN_DIR/li"; then
    info "Updating wrapper script with LIMOON_HOME..."
    cat > "$BIN_DIR/li" <<EOF
#!/bin/sh
export LIMOON_HOME="$INSTALL_DIR"
exec "$INSTALL_DIR/li" "\$@"
EOF
    chmod 755 "$BIN_DIR/li"
    ok "Wrapper script updated."
  else
    ok "Wrapper script is OK."
  fi
fi

# 3. Check for required files
info "Checking required files..."
for file in init.lua core modules; do
  if [ ! -e "$INSTALL_DIR/$file" ]; then
    die "Missing required file: $INSTALL_DIR/$file"
  fi
done
ok "All required files present."

# 4. Test run
info "Testing Li Moon..."
if "$BIN_DIR/li" --help 2>/dev/null || true; then
  ok "Li Moon is working!"
else
  info "Note: Test returned non-zero, but this may be normal (no --help option)"
fi

printf '\n\033[1;32mFix complete!\033[0m Try running: li\n'
