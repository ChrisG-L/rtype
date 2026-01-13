#!/bin/bash
#
# R-Type Client Launcher
# Automatically handles PipeWire/JACK audio setup on Linux
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Detect platform
case "$(uname -s)" in
    Linux*)
        PLATFORM="linux"
        CLIENT_PATH="$PROJECT_ROOT/artifacts/client/linux/rtype_client"
        ;;
    MINGW*|MSYS*|CYGWIN*)
        PLATFORM="windows"
        CLIENT_PATH="$PROJECT_ROOT/artifacts/client/windows/rtype_client.exe"
        ;;
    Darwin*)
        PLATFORM="macos"
        CLIENT_PATH="$PROJECT_ROOT/artifacts/client/macos/rtype_client"
        ;;
    *)
        echo "Unsupported platform"
        exit 1
        ;;
esac

if [[ ! -f "$CLIENT_PATH" ]]; then
    echo "Error: Client not found at $CLIENT_PATH"
    echo "Please compile the project first with: ./scripts/compile.sh"
    exit 1
fi

# On Linux, check if we need pw-jack wrapper for PipeWire
if [[ "$PLATFORM" == "linux" ]]; then
    # Check if PipeWire is running
    if pgrep -x "pipewire" > /dev/null 2>&1; then
        # Check if pw-jack is available
        if command -v pw-jack > /dev/null 2>&1; then
            echo "[Audio] PipeWire detected, using pw-jack wrapper"
            # Set PIPEWIRE_PROPS to enable auto-connection to default sink
            # See: https://docs.pipewire.org/page_man_pipewire-props_7.html
            export PIPEWIRE_PROPS="{ node.autoconnect = true }"
            exec pw-jack "$CLIENT_PATH" "$@"
        else
            echo "[Warning] PipeWire detected but pw-jack not found."
            echo "Install with: sudo apt install pipewire-audio-client-libraries"
            echo "Attempting to run without JACK wrapper..."
        fi
    fi
fi

# Default: run directly
exec "$CLIENT_PATH" "$@"
