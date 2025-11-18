#!/bin/bash

set -e # Arrêter en cas d'erreur

cd "$(git rev-parse --show-toplevel)"

# Compilation du projet en Debug
cmake --build build --config Debug

echo "✅ Compilation terminée avec succès"

# Vérifier si le flag --no-launch est présent
if [[ "$*" != *"--no-launch"* ]]; then
    echo "Lancement du serveur..."
    # Lancer le serveur compilé
    ./artifacts/server/linux/rtype_client
else
    echo "Serveur non lancé (flag --no-launch détecté)"
fi