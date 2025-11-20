#!/bin/bash

set -e # Arrêter en cas d'erreur

# Parse arguments
PLATFORM="linux"  # Default platform
LAUNCH_MODE="server"  # Default: server only (options: server, client, both, none)

for arg in "$@"; do
    case $arg in
        --platform=*)
            PLATFORM="${arg#*=}"
            shift
            ;;
        --client)
            LAUNCH_MODE="client"
            shift
            ;;
        --server)
            LAUNCH_MODE="server"
            shift
            ;;
        --both)
            LAUNCH_MODE="both"
            shift
            ;;
        --no-launch)
            LAUNCH_MODE="none"
            shift
            ;;
        *)
            # Unknown option
            ;;
    esac
done

# Validate platform
if [[ "$PLATFORM" != "linux" && "$PLATFORM" != "windows" && "$PLATFORM" != "macos" ]]; then
    echo "❌ Erreur: Platform invalide '$PLATFORM'. Utilisez: linux, windows, ou macos"
    exit 1
fi

# Déterminer le dossier de build selon la plateforme
case $PLATFORM in
    linux)
        BUILD_DIR="buildLinux"
        SERVER_PATH="./artifacts/server/linux/rtype_server"
        CLIENT_PATH="./artifacts/client/linux/rtype_client"
        ;;
    windows)
        BUILD_DIR="buildWin"
        SERVER_PATH="./artifacts/server/windows/rtype_server.exe"
        CLIENT_PATH="./artifacts/client/windows/rtype_client.exe"
        ;;
    macos)
        BUILD_DIR="buildMac"
        SERVER_PATH="./artifacts/server/macos/rtype_server"
        CLIENT_PATH="./artifacts/client/macos/rtype_client"
        ;;
    *)
        echo "❌ Erreur: Plateforme non supportée: $PLATFORM"
        exit 1
        ;;
esac

cd "$(git rev-parse --show-toplevel)"

# Vérifier que le dossier de build existe
if [[ ! -d "$BUILD_DIR" ]]; then
    echo "❌ Erreur: Le dossier $BUILD_DIR n'existe pas."
    echo "   Veuillez d'abord exécuter: ./scripts/build.sh --platform=$PLATFORM"
    exit 1
fi

echo "🎯 Compilation pour plateforme: $PLATFORM"
echo "📁 Dossier de build: $BUILD_DIR"

# Compilation du projet en Debug
cmake --build "$BUILD_DIR" --config Debug

echo "✅ Compilation terminée avec succès"

SANITIZER_ENV="LSAN_OPTIONS=suppressions=$(pwd)/lsan.supp"

# Fonction pour tuer les instances précédentes
kill_previous() {
    if pgrep -f "rtype_server" > /dev/null 2>&1; then
        echo "🔄 Arrêt du serveur précédent..."
        pkill -f "rtype_server" 2>/dev/null || true
        sleep 0.5
    fi
}

# Lancement selon le mode
if [[ "$PLATFORM" == "linux" || "$PLATFORM" == "macos" ]]; then
    case $LAUNCH_MODE in
        server)
            kill_previous
            echo "🚀 Lancement du serveur..."
            env $SANITIZER_ENV "$SERVER_PATH"
            ;;
        client)
            echo "🎮 Lancement du client..."
            env $SANITIZER_ENV "$CLIENT_PATH"
            ;;
        both)
            kill_previous
            echo "🚀 Lancement du serveur en arrière-plan..."
            env $SANITIZER_ENV "$SERVER_PATH" &
            sleep 1
            echo "🎮 Lancement du client..."
            env $SANITIZER_ENV "$CLIENT_PATH"
            ;;
        none)
            echo "⏭️  Aucun lancement (flag --no-launch détecté)"
            echo "   Serveur: $SERVER_PATH"
            echo "   Client: $CLIENT_PATH"
            ;;
    esac
else
    echo "⚠️  Lancement automatique non disponible pour la plateforme $PLATFORM"
    echo "   Serveur: $SERVER_PATH"
    echo "   Client: $CLIENT_PATH"
fi
