#!/bin/bash
# Script pour construire l'image builder une seule fois
# Usage: ./build_image.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
WORKSPACE="$(cd "${SCRIPT_DIR}/../.." && pwd)"
IMAGE="rtype-builder:latest"

echo "🔨 Construction de l'image builder"
echo "===================================="
echo "  Dockerfile:  ${WORKSPACE}/ci_cd/docker/Dockerfile.build"
echo "  Context:     ${WORKSPACE}"
echo "  Image:       ${IMAGE}"
echo ""

docker build \
    -t "${IMAGE}" \
    -f "${WORKSPACE}/ci_cd/docker/Dockerfile.build" \
    "${WORKSPACE}"

echo ""
echo "✅ Image construite avec succès !"
echo ""
echo "Pour lancer un builder:"
echo "  ./launch_builder.sh [prefix] [port]"
echo ""
echo "Exemples:"
echo "  ./launch_builder.sh build_1_ 8083"
echo "  ./launch_builder.sh build_2_ 8084"
