#!/bin/bash
set -e

# Retourner au répertoire racine du dépôt
cd "$(git rev-parse --show-toplevel)"

PROJECT_ROOT="$(pwd)"
VCPKG_DIR="$PROJECT_ROOT/third_party/vcpkg"
BUILD_DIR="$PROJECT_ROOT/build"

echo "📁 Racine du projet: $PROJECT_ROOT"

# Vérifier que vcpkg est installé
if [ ! -f "$VCPKG_DIR/vcpkg" ]; then
    echo "❌ vcpkg n'est pas installé. Exécutez d'abord install_vcpkg.sh"
    exit 1
fi

# Créer le dossier de build
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configurer le projet avec CMake
echo "⚙️  Configuration du projet avec CMake..."
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE="$VCPKG_DIR/scripts/buildsystems/vcpkg.cmake" \
    -DCMAKE_BUILD_TYPE=Release

# Compiler
echo "🔨 Compilation..."
cmake --build . --config Release

echo "✅ Compilation terminée avec succès!"