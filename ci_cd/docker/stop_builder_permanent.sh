#!/bin/bash
# Script pour stopper et supprimer le builder permanent

set -e

CONTAINER_NAME="rtype_builder"
VOLUME_NAME="rtype_builder_workspace"

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
echo "ℹ️  Le volume ${VOLUME_NAME} a été conservé (données persistantes)"
echo "   Pour supprimer le volume : docker volume rm ${VOLUME_NAME}"
echo ""
echo "Pour redémarrer le builder avec les données conservées :"
echo "  ./launch_builder_permanent.sh"
echo ""
echo "Pour redémarrer proprement (en supprimant les données) :"
echo "  docker volume rm ${VOLUME_NAME}"
echo "  ./launch_builder_permanent.sh"
