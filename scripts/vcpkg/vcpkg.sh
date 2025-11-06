#!/bin/bash
set -e

# Obtenir la racine du projet
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
VCPKG_DIR="$PROJECT_ROOT/third_party/vcpkg"
VCPKG_EXEC="$VCPKG_DIR/vcpkg"

echo "📁 Racine du projet: $PROJECT_ROOT"
echo "📦 Dossier vcpkg: $VCPKG_DIR"

# Vérifications
if [ ! -d "$VCPKG_DIR" ]; then
    echo "❌ Dossier vcpkg non trouvé: $VCPKG_DIR"
    echo "💡 Exécutez: ./scripts/vcpkg/install_vcpkg.sh"
    exit 1
fi

if [ ! -f "$VCPKG_EXEC" ]; then
    echo "❌ Exécutable vcpkg non trouvé: $VCPKG_EXEC"
    echo "💡 Exécutez: ./scripts/vcpkg/install_vcpkg.sh"
    exit 1
fi

# Définir VCPKG_ROOT (important pour vcpkg)
export VCPKG_ROOT="$VCPKG_DIR"

# Se placer dans le répertoire du projet (pour que vcpkg trouve vcpkg.json)
cd "$PROJECT_ROOT"

# Afficher ce qu'on va faire
echo "🚀 Exécution: vcpkg $*"
echo "📂 Working directory: $(pwd)"

# Exécuter vcpkg avec tous les arguments passés
"$VCPKG_EXEC" "$@"

echo "✅ Commande vcpkg terminée avec succès"