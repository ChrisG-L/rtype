#!/bin/bash
# Script pour stopper et nettoyer un conteneur builder
# Usage: ./stop_builder.sh [PREFIX]
# Exemple: ./stop_builder.sh build_1_

set -e

# Paramètres
PREFIX="${1:-local_}"
CONTAINER_NAME="${PREFIX}rtype_builder"
ARTIFACTS_VOLUME="${PREFIX}builder_artifacts"
REMOVE_VOLUME="${2:-false}"

echo "🛑 Arrêt du builder Docker"
echo "================================"
echo "  Container:   ${CONTAINER_NAME}"
echo "  Volume:      ${VOLUME_NAME}"
echo ""

# Vérifier si le conteneur existe
if ! docker ps -a --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
    echo "❌ Le conteneur ${CONTAINER_NAME} n'existe pas"
    exit 1
fi

# Stopper le conteneur s'il tourne
if docker ps --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
    echo "🛑 Arrêt du conteneur..."
    docker stop "${CONTAINER_NAME}"
    echo "✅ Conteneur stoppé"
else
    echo "ℹ️  Le conteneur est déjà arrêté"
fi

# Supprimer le conteneur
echo "🗑️  Suppression du conteneur..."
docker rm "${CONTAINER_NAME}"
echo "✅ Conteneur supprimé"

# Supprimer le volume d'artefacts si demandé
if [ "$REMOVE_VOLUME" = "true" ] || [ "$REMOVE_VOLUME" = "yes" ] || [ "$REMOVE_VOLUME" = "1" ]; then
    if docker volume inspect "${ARTIFACTS_VOLUME}" >/dev/null 2>&1; then
        echo "🗑️  Suppression du volume d'artefacts..."
        docker volume rm "${ARTIFACTS_VOLUME}"
        echo "✅ Volume supprimé"
    else
        echo "ℹ️  Le volume d'artefacts n'existe pas"
    fi
fi

echo ""
echo "✅ Nettoyage terminé !"
