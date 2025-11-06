#!/bin/bash
set -e

PROJECT_ROOT="$(cd "$(git rev-parse --show-toplevel)" && pwd)"
VCPKG_DIR="$PROJECT_ROOT/third_party/vcpkg"
VCPKG_EXEC="$VCPKG_DIR/vcpkg"

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

# Définir VCPKG_ROOT
export VCPKG_ROOT="$VCPKG_DIR"

# Retourner à la racine du projet
cd "$PROJECT_ROOT"

# Exécuter vcpkg
echo "🚀 Exécution: vcpkg $*"
"$VCPKG_EXEC" "$@"
```

## 🎯 Structure finale recommandée
```
rtype/
├── scripts/
│   ├── vcpkg/
│   │   ├── install_vcpkg.sh    # Clone + bootstrap vcpkg
│   │   └── vcpkg.sh            # Wrapper pour exécuter vcpkg
│   └── build.sh                # Configure + compile le projet
├── third_party/
│   └── vcpkg/                  # Installé par install_vcpkg.sh
│       └── vcpkg               # Exécutable compilé
├── vcpkg.json                  # Manifeste des dépendances
└── CMakeLists.txt