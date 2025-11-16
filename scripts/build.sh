#!/bin/bash
set -e  # Arrêter en cas d'erreur

PROJECT_ROOT="$(cd "$(git rev-parse --show-toplevel)" && pwd)"
VCPKG_DIR="$PROJECT_ROOT/third_party/vcpkg"

echo "📁 Installation de vcpkg dans: $VCPKG_DIR"

# Créer le dossier third_party s'il n'existe pas
mkdir -p "$PROJECT_ROOT/third_party"

# Cloner vcpkg s'il n'existe pas, sinon mettre à jour / continuer
if [ -d "$VCPKG_DIR" ]; then
    echo "📁 Le dossier vcpkg existe: $VCPKG_DIR"
    if [ -d "$VCPKG_DIR/.git" ]; then
        echo "🔄 Mise à jour du dépôt vcpkg (pull --ff-only)..."
        # Essayer de récupérer les dernières modifications sans forcer l'échec du script
        git -C "$VCPKG_DIR" pull --ff-only || echo "⚠️ Impossible de faire 'git pull --ff-only' sur vcpkg, on continue"
    else
        echo "⚠️ Le dossier existe mais n'est pas un dépôt git. On continue sans cloner ni mettre à jour."
    fi
else
    echo "📥 Clonage de vcpkg..."
    git clone https://github.com/microsoft/vcpkg.git "$VCPKG_DIR"
fi

# Aller dans le dossier vcpkg
cd "$VCPKG_DIR"

# Compiler vcpkg (bootstrap)
echo "🔨 Compilation de vcpkg (bootstrap)..."
./bootstrap-vcpkg.sh

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