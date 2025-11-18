#!/bin/bash
set -e  # Arrêter en cas d'erreur

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

PROJECT_ROOT="$(cd "$(git rev-parse --show-toplevel)" && pwd)"
VCPKG_DIR="$PROJECT_ROOT/third_party/vcpkg"

echo "🎯 Plateforme cible: $PLATFORM"
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
        "$VCPKG_DIR/vcpkg" update || echo "⚠️ Impossible de faire 'vcpkg update', on continue"
    else
        echo "⚠️ Le dossier vcpkg existe mais n'est pas un dépôt git. On le supprime et on reclone."
        rm -rf "$VCPKG_DIR"
        echo "📥 Clonage de vcpkg..."
        git clone https://github.com/microsoft/vcpkg.git "$VCPKG_DIR"
    fi
else
    echo "📥 Clonage de vcpkg..."
    git clone https://github.com/microsoft/vcpkg.git "$VCPKG_DIR"
fi

# Installer argon2 : sans sudo si on est root, sinon avec sudo
if [ "$(id -u)" -eq 0 ]; then
    apt install -y argon2
else
    sudo apt install -y argon2
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

# Configuration spécifique à la plateforme
case $PLATFORM in
    linux)
        VCPKG_TRIPLET="x64-linux"
        CMAKE_CXX_COMPILER="g++"
        CMAKE_C_COMPILER="gcc"
        CMAKE_EXTRA_FLAGS=""
        ;;
    windows)
        VCPKG_TRIPLET="x64-mingw-static"
        CMAKE_CXX_COMPILER="x86_64-w64-mingw32-g++"
        CMAKE_C_COMPILER="x86_64-w64-mingw32-gcc"
        CMAKE_EXTRA_FLAGS="-DCMAKE_TOOLCHAIN_FILE=$PROJECT_ROOT/triplets/toolchains/mingw-w64-x86_64.cmake"
        ;;
    macos)
        VCPKG_TRIPLET="x64-osx"
        CMAKE_CXX_COMPILER="clang++"
        CMAKE_C_COMPILER="clang"
        CMAKE_EXTRA_FLAGS=""
        ;;
esac

echo "📋 Configuration CMake:"
echo "   - Triplet vcpkg: $VCPKG_TRIPLET"
echo "   - Compilateur C++: $CMAKE_CXX_COMPILER"
echo "   - Compilateur C: $CMAKE_C_COMPILER"

echo "⚙️  Configuration du projet CMake..."
mkdir -p build
cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_MAKE_PROGRAM=/usr/bin/ninja \
    -G "Ninja" \
    -DCMAKE_CXX_COMPILER=$CMAKE_CXX_COMPILER \
    -DCMAKE_C_COMPILER=$CMAKE_C_COMPILER \
    -DVCPKG_TARGET_TRIPLET=$VCPKG_TRIPLET \
    $CMAKE_EXTRA_FLAGS \
    -DCMAKE_TOOLCHAIN_FILE=third_party/vcpkg/scripts/buildsystems/vcpkg.cmake

echo "✅ Configuration terminée pour plateforme: $PLATFORM"