#!/bin/bash
# Script pour stopper et supprimer le builder permanent

set -e

CONTAINER_NAME="rtype_builder"

echo "🛑 Arrêt du Builder Permanent"
echo "============================="

if docker ps --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
    echo "🛑 Arrêt du conteneur ${CONTAINER_NAME}..."
    docker stop "${CONTAINER_NAME}"
    echo "✅ Conteneur arrêté"
fi

if docker ps -a --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
    echo "🗑️  Suppression du conteneur ${CONTAINER_NAME}..."
    docker rm "${CONTAINER_NAME}"
    echo "✅ Conteneur supprimé"
else
    echo "ℹ️  Le conteneur ${CONTAINER_NAME} n'existe pas"
fi

echo ""
echo "✅ Nettoyage terminé"
echo ""
echo "ℹ️  Pour redémarrer le builder :"
echo "  ./launch_builder_permanent.sh"
