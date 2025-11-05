#!/bin/bash
set -e

echo "bash source: ${BASH_SOURCE[0]}"
PROJECT_ROOT="$(cd "$(git rev-parse --show-toplevel)" && pwd)"
echo "📁 Racine du projet: $PROJECT_ROOT"
VCPKG_DIR="$PROJECT_ROOT/third_party/vcpkg"
PROJECT_MANIFEST="$PROJECT_ROOT/vcpkg.json"

# Vérifications
if [ ! -d "$VCPKG_DIR" ]; then
    echo "❌ Dossier vcpkg non trouvé: $VCPKG_DIR"
    exit 1
fi

if [ ! -f "$VCPKG_DIR/vcpkg" ]; then
    echo "❌ Exécutable vcpkg non trouvé"
    exit 1
fi

# Info sur le manifeste du projet
if [ -f "$PROJECT_MANIFEST" ]; then
    echo "📄 Manifeste du projet: $PROJECT_MANIFEST"
else
    echo "⚠️  Pas de vcpkg.json à la racine du projet"
    echo "   Créez-le avec: ./scripts/vcpkg.sh new --application"
fi

# Aller dans le dossier vcpkg
cd "$VCPKG_DIR"

# Définir VCPKG_ROOT
export VCPKG_ROOT="$(pwd)"

# IMPORTANT : Retourner à la racine du projet avant d'exécuter
# (pour que vcpkg trouve le vcpkg.json du projet)
cd "$PROJECT_ROOT"

# Exécuter vcpkg avec le bon contexte
"$VCPKG_DIR/vcpkg" "$@"