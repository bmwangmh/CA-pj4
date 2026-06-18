#!/usr/bin/env bash
#
# setup-mac.sh — one-shot build helper for the Longan Nano Snake project on macOS.
#
# Why this exists: PlatformIO pulled the official gd32v packages from its
# registry (HTTP 403). We get the macOS toolchain from an unofficial fork
# (already set in platformio.ini) and borrow the OS-independent framework
# `framework-gd32vf103-sdk` from the course offline dev-kit.
#
# Usage:
#   ./setup-mac.sh                 # set up + compile
#   ./setup-mac.sh upload          # set up + compile + DFU flash
#   DEVKIT=/path/to/extracted ./setup-mac.sh   # if auto-search can't find it
#
set -euo pipefail

# ---- locate the project (this script lives in snake/) -----------------------
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]:-$0}")" && pwd)"
PIO_PKGS="$HOME/.platformio/packages"
FRAMEWORK_DST="$PIO_PKGS/framework-gd32vf103-sdk"
TOOLCHAIN_DST="$PIO_PKGS/toolchain-gd32v-mac"
TOOLCHAIN_URL="https://github.com/yesitsme007/platformio-gd32v-mac-unofficial-repo/releases/download/0.0.1/riscv64-unknown-elf-gcc-8.3.0-2019.08.0-x86_64-apple-darwin.tar.gz"

say()  { printf '\033[1;36m==> %s\033[0m\n' "$*"; }
warn() { printf '\033[1;33m!!  %s\033[0m\n' "$*"; }
die()  { printf '\033[1;31mERROR: %s\033[0m\n' "$*" >&2; exit 1; }

# ---- 0. checks --------------------------------------------------------------
command -v pio >/dev/null 2>&1 || die "pio not found in PATH. Install PlatformIO first (brew install platformio)."
command -v git >/dev/null 2>&1 || die "git not found."

# ---- 1. Rosetta (Apple Silicon needs it for the x86_64 toolchain) -----------
if [ "$(uname -m)" = "arm64" ]; then
  if /usr/bin/pgrep -q oahd || arch -x86_64 /usr/bin/true 2>/dev/null; then
    say "Rosetta already available."
  else
    say "Installing Rosetta 2 (needed to run the x86_64 RISC-V compiler)..."
    softwareupdate --install-rosetta --agree-to-license || \
      warn "Rosetta install failed; if the compiler won't run, install it manually."
  fi
fi

# ---- 2. make sure the framework package is installed ------------------------
if [ -f "$FRAMEWORK_DST/package.json" ]; then
  say "framework-gd32vf103-sdk already installed at $FRAMEWORK_DST — skipping copy."
else
  say "Looking for framework-gd32vf103-sdk in the course dev-kit..."
  FRAMEWORK_SRC=""

  # honour an explicit DEVKIT hint first
  if [ -n "${DEVKIT:-}" ]; then
    FRAMEWORK_SRC="$(find "$DEVKIT" -type d -name framework-gd32vf103-sdk 2>/dev/null | head -n1 || true)"
  fi

  # otherwise search common locations
  if [ -z "$FRAMEWORK_SRC" ]; then
    for root in "$HOME/Downloads" "$HOME/Desktop" "$HOME" "$(dirname "$PROJECT_DIR")"; do
      [ -d "$root" ] || continue
      FRAMEWORK_SRC="$(find "$root" -maxdepth 6 -type d -name framework-gd32vf103-sdk 2>/dev/null | head -n1 || true)"
      [ -n "$FRAMEWORK_SRC" ] && break
    done
  fi

  [ -n "$FRAMEWORK_SRC" ] && [ -f "$FRAMEWORK_SRC/package.json" ] || die \
"Could not find framework-gd32vf103-sdk on your machine.
 Extract the course dev-kit first, e.g.:
   cd ~/Downloads/lab11_lab12_project4
   tar xf longan-dev-kit-ca1-linux.tar.xz
 then re-run, optionally pointing at it:
   DEVKIT=~/Downloads/lab11_lab12_project4 $0"

  say "Found: $FRAMEWORK_SRC"
  mkdir -p "$PIO_PKGS"
  cp -R "$FRAMEWORK_SRC" "$FRAMEWORK_DST"
  say "Copied framework -> $FRAMEWORK_DST"
  grep '"name"\|"version"' "$FRAMEWORK_DST/package.json" || true
fi

# ---- 2b. make sure the macOS toolchain is installed -------------------------
# The fork hosts it on its own GitHub release, which modern PlatformIO can't
# resolve automatically — so we fetch and drop it in by hand, same as the
# framework. (curl downloads aren't Gatekeeper-quarantined, so it just runs.)
if [ -f "$TOOLCHAIN_DST/package.json" ] && [ -f "$TOOLCHAIN_DST/bin/riscv64-unknown-elf-gcc" ]; then
  say "toolchain-gd32v-mac already present — skipping download."
else
  say "Downloading the macOS RISC-V toolchain (~200 MB)..."
  tmp="$(mktemp -d)"
  curl -L --fail -o "$tmp/tc.tar.gz" "$TOOLCHAIN_URL" || die "toolchain download failed."
  say "Extracting toolchain..."
  tar xzf "$tmp/tc.tar.gz" -C "$tmp"
  bindir="$(find "$tmp" -maxdepth 4 -type d -name bin | head -n1 || true)"
  [ -n "$bindir" ] || die "could not locate bin/ inside the toolchain archive."
  root="$(dirname "$bindir")"
  rm -rf "$TOOLCHAIN_DST"
  mkdir -p "$TOOLCHAIN_DST"
  cp -R "$root"/. "$TOOLCHAIN_DST"/
  # PlatformIO needs a manifest with a matching name/version; no "system"
  # field so it's accepted on arm64 (binaries run under Rosetta).
  printf '{"name": "toolchain-gd32v-mac", "version": "9.2.0"}\n' > "$TOOLCHAIN_DST/package.json"
  rm -rf "$tmp"
  say "Toolchain installed -> $TOOLCHAIN_DST"
fi
# Always ensure the binaries are runnable (extraction can drop the +x bit) and
# not quarantined, otherwise gcc fails with "Permission denied" (Error 126).
chmod -R +x "$TOOLCHAIN_DST" 2>/dev/null || true
xattr -dr com.apple.quarantine "$TOOLCHAIN_DST" 2>/dev/null || true

# ---- 3. build ---------------------------------------------------------------
say "Building..."
cd "$PROJECT_DIR"
pio run

# ---- 4. optional flash (via dfu-util, the method proven on this board) -------
FW="$PROJECT_DIR/.pio/build/sipeed-longan-nano/firmware.bin"
if [ "${1:-}" = "upload" ] || [ "${1:-}" = "flash" ]; then
  command -v dfu-util >/dev/null 2>&1 || die "dfu-util not found (brew install dfu-util)."
  [ -f "$FW" ] || die "built firmware not found at $FW"
  echo
  warn "Put the board in DFU mode: hold BOOT0, tap RESET, release BOOT0."
  read -r -p "Press Enter once the board appears in 'dfu-util -l'... " _
  say "Flashing $FW ..."
  # The two messages 'Invalid DFU suffix signature' and
  # 'Error during download get_status' are EXPECTED on GD32 — ignore them.
  dfu-util -a 0 --dfuse-address 0x08000000:leave -D "$FW" || true
  warn "Press the RESET button on the board to exit DFU and run the game."
fi

say "Done."
