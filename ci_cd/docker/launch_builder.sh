#!/bin/bash
# Script pour lancer un conteneur builder avec des paramètres personnalisés
# Usage: ./launch_builder.sh [PREFIX] [PORT]
# Exemple: ./launch_builder.sh build_1_ 8083

set -e

# Paramètres par défaut
PREFIX="${1:-local_}"
BUILDER_PORT="${2:-8080}"
WORKSPACE="${3:-$(cd "$(dirname "$0")/../.." && pwd)}"

# Nom du conteneur et nom du volume pour les artefacts (logs)
CONTAINER_NAME="${PREFIX}rtype_builder"
ARTIFACTS_VOLUME="${PREFIX}builder_artifacts"

# Image à utiliser (toujours latest, pas de rebuild)
IMAGE="rtype-builder:latest"

echo "🐳 Lancement du builder Docker"
echo "================================"
echo "  Image:       ${IMAGE}"
echo "  Container:   ${CONTAINER_NAME}"
echo "  Volume:      ${ARTIFACTS_VOLUME}"
echo "  Port:        ${BUILDER_PORT}:8080"
echo "  Workspace:   ${WORKSPACE}"
echo ""

# Vérifier si l'image existe
if ! docker image inspect "${IMAGE}" >/dev/null 2>&1; then
    echo "❌ Image ${IMAGE} introuvable. Construction de l'image..."
    docker build -t "${IMAGE}" -f "${WORKSPACE}/ci_cd/docker/Dockerfile.build" "${WORKSPACE}"
    echo "✅ Image construite"
fi

# Vérifier si le conteneur existe déjà
if docker ps -a --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
    echo "⚠️  Le conteneur ${CONTAINER_NAME} existe déjà"
    
    # Vérifier s'il tourne
    if docker ps --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
        echo "✅ Le conteneur est déjà en cours d'exécution"
        echo ""
        echo "Pour le redémarrer, utilisez:"
        echo "  docker restart ${CONTAINER_NAME}"
        echo ""
        echo "Pour le stopper:"
        echo "  docker stop ${CONTAINER_NAME}"
        exit 0
    else
        echo "🔄 Démarrage du conteneur existant..."
        docker start "${CONTAINER_NAME}"
        echo "✅ Conteneur démarré"
        exit 0
    fi
fi

# Créer le volume d'artefacts s'il n'existe pas (utilisé pour logs, résultats)
if ! docker volume inspect "${ARTIFACTS_VOLUME}" >/dev/null 2>&1; then
    echo "📦 Création du volume d'artefacts ${ARTIFACTS_VOLUME}..."
    docker volume create "${ARTIFACTS_VOLUME}"
fi

# Lancer le conteneur
echo "🚀 Lancement du conteneur (workspace monté en lecture seule)..."
docker run -d \
    --name "${CONTAINER_NAME}" \
    -p "${BUILDER_PORT}:8080" \
    -v "${WORKSPACE}:/workspace" \
    -v "${ARTIFACTS_VOLUME}:/workspace/artifacts" \
    -w /workspace \
    -e WORKSPACE=/workspace \
    -e BUILDER_PORT=8080 \
    "${IMAGE}"

echo ""
echo "✅ Builder lancé avec succès !"
echo ""
echo "🔗 API disponible sur: http://localhost:${BUILDER_PORT}"
echo ""
echo "Commandes utiles:"
echo "  - Vérifier la santé:    curl http://localhost:${BUILDER_PORT}/health"
echo "  - Voir les logs:        docker logs -f ${CONTAINER_NAME}"
echo "  - Stopper:              docker stop ${CONTAINER_NAME}"
echo "  - Supprimer:            docker rm ${CONTAINER_NAME}"
echo "  - Supprimer + volume:   docker rm ${CONTAINER_NAME} && docker volume rm ${ARTIFACTS_VOLUME}"
