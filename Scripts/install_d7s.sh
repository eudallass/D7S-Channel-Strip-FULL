#!/usr/bin/env bash
set -euo pipefail

ZIP_PATH="${1:-$HOME/Downloads/D7S-VST3-Mac-ARM64.zip}"
PLUGIN_DIR="/Library/Audio/Plug-Ins/VST3"
PLUGIN_NAME="D7S Channel Strip FULL.vst3"

if pgrep -x "Live" >/dev/null 2>&1; then
  echo "Closing Ableton Live..."
  killall Live || true
fi

if [[ ! -f "$ZIP_PATH" ]]; then
  echo "ZIP not found: $ZIP_PATH"
  exit 1
fi

TEMP_DIR="$(mktemp -d)"
cleanup() { rm -rf "$TEMP_DIR"; }
trap cleanup EXIT

unzip -o "$ZIP_PATH" -d "$TEMP_DIR"
FOUND_PLUGIN="$(find "$TEMP_DIR" -name "$PLUGIN_NAME" -type d | head -n 1)"

if [[ -z "$FOUND_PLUGIN" ]]; then
  echo "Plugin bundle not found inside ZIP."
  exit 1
fi

echo "Installing $PLUGIN_NAME..."
sudo rm -rf "$PLUGIN_DIR/$PLUGIN_NAME"
sudo mkdir -p "$PLUGIN_DIR"
sudo cp -R "$FOUND_PLUGIN" "$PLUGIN_DIR/"
sudo xattr -rd com.apple.quarantine "$PLUGIN_DIR/$PLUGIN_NAME" || true

LIVE_DB="$HOME/Library/Application Support/Ableton/Live Database/Live-plugins-1.db"
if [[ -f "$LIVE_DB" ]] && command -v sqlite3 >/dev/null 2>&1; then
  sqlite3 "$LIVE_DB" "DELETE FROM plugins WHERE module_id IN (SELECT module_id FROM plugin_modules WHERE path LIKE '%D7S Channel Strip FULL.vst3%'); DELETE FROM plugin_modules WHERE path LIKE '%D7S Channel Strip FULL.vst3%';" || true
fi

echo "✅ D7S Channel Strip FULL instalado. Abra o Ableton e faça rescan se necessário."
