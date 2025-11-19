#!/bin/bash

set -e # Arrêter en cas d'erreur

# Parse arguments
PLATFORM="linux"  # Default platform
for arg in "$@"; do
    case $arg in
        --platform=*)
            PLATFORM="${arg#*=}"
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
        ARTIFACT_PATH="./artifacts/server/linux/rtype_server"
        ;;
    windows)
        BUILD_DIR="buildWin"
        ARTIFACT_PATH="./artifacts/server/windows/rtype_server.exe"
        ;;
    macos)
        BUILD_DIR="buildMac"
        ARTIFACT_PATH="./artifacts/server/macos/rtype_server"
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

# Vérifier si le flag --no-launch est présent
if [[ "$*" != *"--no-launch"* ]]; then
    if [[ "$PLATFORM" == "linux" || "$PLATFORM" == "macos" ]]; then
        echo "🚀 Lancement du serveur..."
        # Lancer le serveur compilé
        "$ARTIFACT_PATH"
    else
        echo "⚠️  Lancement automatique non disponible pour la plateforme $PLATFORM"
        echo "   L'exécutable est disponible dans: $ARTIFACT_PATH"
    fi
else
    echo "⏭️  Serveur non lancé (flag --no-launch détecté)"
fi