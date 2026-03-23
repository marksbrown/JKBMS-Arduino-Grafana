#!/bin/bash
# Build script for JKBMS-Arduino-Grafana
# Target: Arduino Uno R4 WiFi

set -e

FQBN="arduino:renesas_uno:unor4wifi"
SKETCH_DIR="bms"          # contains bms.ino + arduino_secrets.h
BUILD_OUTPUT="build/output"

# ── Colours ────────────────────────────────────────────────────────────────
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; NC='\033[0m'
info()  { echo -e "${GREEN}[INFO]${NC}  $*"; }
warn()  { echo -e "${YELLOW}[WARN]${NC}  $*"; }
error() { echo -e "${RED}[ERROR]${NC} $*"; exit 1; }

# ── 1. Check for arduino_secrets.h ─────────────────────────────────────────
if [ ! -f "$SKETCH_DIR/arduino_secrets.h" ]; then
    error "$SKETCH_DIR/arduino_secrets.h not found.\nCopy $SKETCH_DIR/arduino_secrets.h.example to $SKETCH_DIR/arduino_secrets.h and fill in your credentials."
fi

# ── 2. Install arduino-cli if missing ──────────────────────────────────────
if ! command -v arduino-cli &>/dev/null; then
    warn "arduino-cli not found. Installing to ~/.local/bin ..."
    mkdir -p "$HOME/.local/bin"
    curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR="$HOME/.local/bin" sh
    export PATH="$HOME/.local/bin:$PATH"
    info "arduino-cli installed. Add ~/.local/bin to your PATH permanently."
fi

info "arduino-cli $(arduino-cli version)"

# ── 3. Install board package ────────────────────────────────────────────────
info "Updating board index..."
arduino-cli core update-index

if ! arduino-cli core list | grep -q "arduino:renesas_uno"; then
    info "Installing Arduino Renesas Uno board package..."
    arduino-cli core install arduino:renesas_uno
else
    info "Board package arduino:renesas_uno already installed."
fi

# ── 4. Install required libraries ──────────────────────────────────────────
for lib in "ArduinoHttpClient" "ArduinoJson"; do
    if ! arduino-cli lib list | grep -q "^$lib"; then
        info "Installing library: $lib"
        arduino-cli lib install "$lib"
    else
        info "Library already installed: $lib"
    fi
done

# ── 5. Install JKBMSInterface as a local library ───────────────────────────
ARDUINO_DATA_DIR="$(arduino-cli config get directories.data 2>/dev/null || echo "$HOME/.arduino15")"
ARDUINO_USER_DIR="$(arduino-cli config get directories.user 2>/dev/null || echo "$HOME/Arduino")"
ARDUINO_LIBS_DIR="$ARDUINO_USER_DIR/libraries"

JKBMS_DEST="$ARDUINO_LIBS_DIR/JKBMSInterface"
if [ ! -d "$JKBMS_DEST" ]; then
    info "Installing JKBMSInterface library to $JKBMS_DEST"
    mkdir -p "$ARDUINO_LIBS_DIR"
    cp -r JKBMSInterface "$JKBMS_DEST"
else
    info "JKBMSInterface already installed — syncing from local copy..."
    rsync -a --exclude='.git' JKBMSInterface/. "$JKBMS_DEST/"
fi

# ── 6. Compile ─────────────────────────────────────────────────────────────
mkdir -p "$BUILD_OUTPUT"
info "Compiling sketch ($SKETCH_DIR/bms.ino) for $FQBN..."
arduino-cli compile \
    --fqbn "$FQBN" \
    --build-path "$(pwd)/$BUILD_OUTPUT" \
    "$(pwd)/$SKETCH_DIR"

info "Build successful. Firmware in $BUILD_OUTPUT/"

# ── 7. Optional: upload ────────────────────────────────────────────────────
if [ "$1" = "--upload" ]; then
    PORT="${2:-/dev/ttyACM0}"
    info "Uploading to $PORT..."
    arduino-cli upload \
        --fqbn "$FQBN" \
        --port "$PORT" \
        --input-dir "$(pwd)/$BUILD_OUTPUT" \
        "$(pwd)/$SKETCH_DIR"
    info "Upload complete."
fi
