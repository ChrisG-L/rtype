#!/bin/bash

set -e # Arrêter en cas d'erreur

cd "$(git rev-parse --show-toplevel)"

# Compilation du projet en Debug
cmake --build build --config Debug

echo "✅ Compilation terminée avec succès"
echo "Lancement du serveur..."
# Lancer le serveur compilé
./artifacts/server/linux/rtype_server