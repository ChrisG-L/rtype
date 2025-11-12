#!/bin/bash
set -e  # Arrêter en cas d'erreur

PROJECT_ROOT="$(cd "$(git rev-parse --show-toplevel)" && pwd)"
VCPKG_DIR="$PROJECT_ROOT/third_party/vcpkg"

echo "📁 Installation de vcpkg dans: $VCPKG_DIR"

# Créer le dossier third_party s'il n'existe pas
mkdir -p "$PROJECT_ROOT/third_party"

# Cloner vcpkg s'il n'existe pas
echo "📥 Clonage de vcpkg..."
git clone https://github.com/microsoft/vcpkg.git "$VCPKG_DIR"

# Aller dans le dossier vcpkg
cd "$VCPKG_DIR"

# Compiler vcpkg (bootstrap) s'il n'est pas déjà compilé
if [ ! -f "$VCPKG_DIR/vcpkg" ]; then
    echo "🔨 Compilation de vcpkg (bootstrap)..."
    ./bootstrap-vcpkg.sh
else
    echo "✓ vcpkg déjà compilé"
fi

# Vérifier que l'exécutable existe
if [ -f "$VCPKG_DIR/vcpkg" ]; then
    echo "✅ vcpkg installé avec succès!"
    "$VCPKG_DIR/vcpkg" version
else
    echo "❌ Erreur: vcpkg n'a pas été compilé correctement"
    exit 1
fi

echo "pwd: $(pwd)"
cd "../../"
echo "pwd: $(pwd)"

echo "🧹 Nettoyage du dossier build..."
rm -rf build
mkdir -p build

echo "Configuration du projet CMake"
mkdir -p build
cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_MAKE_PROGRAM=/usr/bin/ninja \
    -G "Ninja" \
    -DCMAKE_CXX_COMPILER=g++ \
    -DCMAKE_C_COMPILER=gcc \
    -DVCPKG_TARGET_TRIPLET=x64-linux \
    -DCMAKE_TOOLCHAIN_FILE=third_party/vcpkg/scripts/buildsystems/vcpkg.cmake