#!/bin/bash
# Script pour lancer le builder permanent
# Ce builder est unique et gère plusieurs builds en parallèle via des workspaces isolés

set -e

CONTAINER_NAME="rtype_builder"
IMAGE="rtype-builder:latest"
NETWORK="rtype_ci_network"
API_PORT="8082"      # Port hôte pour l'API HTTP
RSYNC_PORT="8873"  # Port hôte pour rsync (éviter 873 qui peut être réservé)

echo "🐳 Lancement du Builder Permanent"
echo "=================================="
echo "  Image:       ${IMAGE}"
echo "  Container:   ${CONTAINER_NAME}"
echo "  Network:     ${NETWORK}"
echo "  API Port:    ${API_PORT}"
echo "  Rsync Port:  ${RSYNC_PORT}"
echo ""

# Vérifier que le network existe
if ! docker network inspect "${NETWORK}" >/dev/null 2>&1; then
    echo "❌ Network ${NETWORK} introuvable."
    echo "   Assurez-vous que Jenkins est démarré (docker-compose up)"
    exit 1
fi

# Vérifier si le conteneur existe déjà
if docker ps -a --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
    echo "⚠️  Le conteneur ${CONTAINER_NAME} existe déjà"

    # Vérifier s'il tourne
    if docker ps --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
        echo "✅ Le conteneur est déjà en cours d'exécution"
        exit 0
    else
        echo "🗑️  Suppression de l'ancien conteneur..."
        docker rm "${CONTAINER_NAME}"
    fi
fi

# Vérifier si l'image existe
if ! docker image inspect "${IMAGE}" >/dev/null 2>&1; then
    echo "❌ Image ${IMAGE} introuvable."
    echo "   Construisez d'abord l'image avec build_imageBuilder.sh"
    exit 1
fi

# Lancer le conteneur permanent
echo "🚀 Lancement du conteneur..."
docker run -d \
    --name "${CONTAINER_NAME}" \
    --network "${NETWORK}" \
    -p "${API_PORT}:8082" \
    -p "${RSYNC_PORT}:873" \
    -e WORKSPACE=/workspace \
    -e BUILDER_PORT=8082 \
    --restart unless-stopped \
    "${IMAGE}"

# Attendre que le serveur démarre
echo "⏳ Attente du démarrage du serveur..."
sleep 5

# Health check
MAX_RETRIES=10
RETRY=0
while [[ $RETRY -lt $MAX_RETRIES ]]; do
    if curl -s "http://${CONTAINER_NAME}:${API_PORT}/health" >/dev/null 2>&1; then
        echo ""
        echo "✅ Builder permanent lancé avec succès !"
        echo ""
        echo "🔗 API HTTP:   http://${CONTAINER_NAME}:${API_PORT}"
        echo "🔗 Rsync:      rsync://${CONTAINER_NAME}:${RSYNC_PORT}/workspace"
        echo "🔗 Container:  ${CONTAINER_NAME} (network: ${NETWORK})"
        echo ""
        echo "Commandes utiles:"
        echo "  - Voir les logs:      docker logs -f ${CONTAINER_NAME}"
        echo "  - Health check:       curl http://${CONTAINER_NAME}:${API_PORT}/health"
        echo "  - Stopper:            docker stop ${CONTAINER_NAME}"
        echo ""
        echo "API Endpoints disponibles:"
        echo "  - POST /workspace/create"
        echo "  - POST /workspace/{id}/run"
        echo "  - GET  /status/{job_id}"
        echo "  - DELETE /workspace/{id}"
        exit 0
    fi

    RETRY=$((RETRY + 1))
    echo "  Tentative ${RETRY}/${MAX_RETRIES}..."
    sleep 2
done

echo ""
echo "❌ Erreur: Le serveur ne répond pas au health check"
echo "   Vérifiez les logs: docker logs ${CONTAINER_NAME}"
exit 1
