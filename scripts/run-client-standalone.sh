#!/bin/bash
#
# R-Type Client Launcher (Standalone)
# Automatically handles PipeWire/JACK audio setup on Linux
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CLIENT_PATH="$SCRIPT_DIR/rtype_client"

if [[ ! -f "$CLIENT_PATH" ]]; then
    echo "Error: Client not found at $CLIENT_PATH"
    exit 1
fi

# On Linux, check if we need pw-jack wrapper for PipeWire
if [[ "$(uname -s)" == "Linux" ]]; then
    # Check if PipeWire is running
    if pgrep -x "pipewire" > /dev/null 2>&1; then
        # Check if pw-jack is available
        if command -v pw-jack > /dev/null 2>&1; then
            echo "[Audio] PipeWire detected, using pw-jack wrapper"
            export PIPEWIRE_PROPS="{ node.autoconnect = true }"
            exec pw-jack "$CLIENT_PATH" "$@"
        else
            echo "[Warning] PipeWire detected but pw-jack not found."
            echo "Install with: sudo apt install pipewire-jack"
            echo "Attempting to run without JACK wrapper (voice chat may not work)..."
        fi
    fi
fi

# Default: run directly
exec "$CLIENT_PATH" "$@"
