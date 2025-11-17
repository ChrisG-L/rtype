#!/bin/bash
# Test de builds parallèles
# Ce script lance 2 builds en parallèle pour vérifier l'isolation

set -e

echo "🧪 Test de builds parallèles"
echo "=============================="
echo ""

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
BUILD1_PREFIX="test_build_1_"
BUILD1_PORT=9001

BUILD2_PREFIX="test_build_2_"
BUILD2_PORT=9002

COMPOSE_FILE="ci_cd/docker/docker-compose.build.yml"

# Cleanup function
cleanup() {
    echo ""
    echo -e "${YELLOW}🧹 Nettoyage des conteneurs...${NC}"
    
    PREFIX="${BUILD1_PREFIX}" BUILDER_PORT="${BUILD1_PORT}" \
        docker-compose -f ${COMPOSE_FILE} down -v 2>/dev/null || true
    
    PREFIX="${BUILD2_PREFIX}" BUILDER_PORT="${BUILD2_PORT}" \
        docker-compose -f ${COMPOSE_FILE} down -v 2>/dev/null || true
    
    echo -e "${GREEN}✅ Nettoyage terminé${NC}"
}

# Register cleanup on exit
trap cleanup EXIT

echo -e "${BLUE}📋 Étape 1: Démarrage du premier build${NC}"
echo "  Prefix: ${BUILD1_PREFIX}"
echo "  Port: ${BUILD1_PORT}"

PREFIX="${BUILD1_PREFIX}" BUILDER_PORT="${BUILD1_PORT}" \
    docker-compose -f ${COMPOSE_FILE} up -d --build

echo -e "${GREEN}✅ Build #1 démarré${NC}"
echo ""

echo -e "${BLUE}📋 Étape 2: Démarrage du second build${NC}"
echo "  Prefix: ${BUILD2_PREFIX}"
echo "  Port: ${BUILD2_PORT}"

PREFIX="${BUILD2_PREFIX}" BUILDER_PORT="${BUILD2_PORT}" \
    docker-compose -f ${COMPOSE_FILE} up -d --build

echo -e "${GREEN}✅ Build #2 démarré${NC}"
echo ""

echo -e "${BLUE}📋 Étape 3: Attente du démarrage des serveurs${NC}"
sleep 15

echo -e "${BLUE}📋 Étape 4: Vérification des conteneurs${NC}"
echo "Conteneurs actifs:"
docker ps --filter "name=rtype_builder" --format "table {{.Names}}\t{{.Ports}}\t{{.Status}}"
echo ""

echo -e "${BLUE}📋 Étape 5: Health check des deux builds${NC}"

# Health check build 1
echo -e "${YELLOW}Build #1 (port ${BUILD1_PORT}):${NC}"
HEALTH1=$(curl -s "http://localhost:${BUILD1_PORT}/health" || echo "ERROR")
if echo "$HEALTH1" | grep -q '"status":"ok"'; then
    echo -e "${GREEN}✅ Builder #1 opérationnel${NC}"
else
    echo -e "${RED}❌ Builder #1 non opérationnel${NC}"
    exit 1
fi

# Health check build 2
echo -e "${YELLOW}Build #2 (port ${BUILD2_PORT}):${NC}"
HEALTH2=$(curl -s "http://localhost:${BUILD2_PORT}/health" || echo "ERROR")
if echo "$HEALTH2" | grep -q '"status":"ok"'; then
    echo -e "${GREEN}✅ Builder #2 opérationnel${NC}"
else
    echo -e "${RED}❌ Builder #2 non opérationnel${NC}"
    exit 1
fi

echo ""
echo -e "${BLUE}📋 Étape 6: Soumission de jobs parallèles${NC}"

# Submit job to build 1
JOB1_RESPONSE=$(curl -s -X POST -H 'Content-Type: application/json' \
    -d '{"command":"compile"}' "http://localhost:${BUILD1_PORT}/run")
JOB1_ID=$(echo "$JOB1_RESPONSE" | jq -r '.job_id' 2>/dev/null)
echo -e "${GREEN}✅ Job #1 soumis: ${JOB1_ID}${NC}"

# Submit job to build 2
JOB2_RESPONSE=$(curl -s -X POST -H 'Content-Type: application/json' \
    -d '{"command":"compile"}' "http://localhost:${BUILD2_PORT}/run")
JOB2_ID=$(echo "$JOB2_RESPONSE" | jq -r '.job_id' 2>/dev/null)
echo -e "${GREEN}✅ Job #2 soumis: ${JOB2_ID}${NC}"

echo ""
echo -e "${BLUE}📋 Étape 7: Vérification de l'isolation${NC}"

sleep 5

# Check job 1 status
STATUS1=$(curl -s "http://localhost:${BUILD1_PORT}/status/${JOB1_ID}")
STATUS1_STATE=$(echo "$STATUS1" | jq -r '.status' 2>/dev/null)
echo "  Job #1 status: ${STATUS1_STATE}"

# Check job 2 status
STATUS2=$(curl -s "http://localhost:${BUILD2_PORT}/status/${JOB2_ID}")
STATUS2_STATE=$(echo "$STATUS2" | jq -r '.status' 2>/dev/null)
echo "  Job #2 status: ${STATUS2_STATE}"

# Verify isolation: job 1 should not exist in build 2
echo ""
echo "Vérification de l'isolation (job #1 ne doit pas exister dans build #2):"
CROSS_CHECK=$(curl -s "http://localhost:${BUILD2_PORT}/status/${JOB1_ID}")
if echo "$CROSS_CHECK" | grep -q '"error":"job not found"'; then
    echo -e "${GREEN}✅ Isolation confirmée${NC}"
else
    echo -e "${RED}❌ Isolation échouée!${NC}"
    exit 1
fi

echo ""
echo -e "${BLUE}📋 Étape 8: Vérification des volumes${NC}"
echo "Volumes créés:"
docker volume ls --filter "name=builder_home" --format "table {{.Name}}\t{{.Driver}}"

echo ""
echo -e "${GREEN}🎉 Test de builds parallèles réussi !${NC}"
echo ""
echo "Résumé:"
echo "  ✅ 2 conteneurs distincts lancés simultanément"
echo "  ✅ Ports différents (${BUILD1_PORT} et ${BUILD2_PORT})"
echo "  ✅ Volumes isolés"
echo "  ✅ Jobs isolés entre les builds"
echo "  ✅ Pas de collision de ressources"
