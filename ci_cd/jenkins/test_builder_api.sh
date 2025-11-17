#!/bin/bash
# Script de test pour valider l'intégration Builder API
# Usage: ./test_builder_api.sh

set -e

echo "🧪 Test de l'intégration Builder API"
echo "===================================="
echo ""

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Configuration
BUILDER_URL="http://localhost:8080"
MAX_WAIT=120  # 2 minutes pour ce test

echo -e "${YELLOW}📋 Étape 1: Vérification de la santé du builder${NC}"
HEALTH_RESPONSE=$(curl -s "${BUILDER_URL}/health" || echo "ERROR")

if echo "$HEALTH_RESPONSE" | grep -q '"status":"ok"'; then
    echo -e "${GREEN}✅ Builder opérationnel${NC}"
    echo "$HEALTH_RESPONSE" | jq '.' 2>/dev/null || echo "$HEALTH_RESPONSE"
else
    echo -e "${RED}❌ Builder non opérationnel${NC}"
    echo "Assurez-vous que le conteneur est démarré:"
    echo "  docker-compose -f ci_cd/docker/docker-compose.build.yml up -d"
    exit 1
fi

echo ""
echo -e "${YELLOW}📋 Étape 2: Soumission d'un job de test (compile)${NC}"
JOB_RESPONSE=$(curl -s -X POST -H 'Content-Type: application/json' \
    -d '{"command":"compile"}' "${BUILDER_URL}/run")

JOB_ID=$(echo "$JOB_RESPONSE" | jq -r '.job_id' 2>/dev/null)

if [ -z "$JOB_ID" ] || [ "$JOB_ID" = "null" ]; then
    echo -e "${RED}❌ Échec de soumission du job${NC}"
    echo "$JOB_RESPONSE"
    exit 1
fi

echo -e "${GREEN}✅ Job soumis avec UUID: ${JOB_ID}${NC}"

echo ""
echo -e "${YELLOW}📋 Étape 3: Attente de la completion (max ${MAX_WAIT}s)${NC}"

START_TIME=$(date +%s)
POLL_INTERVAL=5

while true; do
    STATUS_RESPONSE=$(curl -s "${BUILDER_URL}/status/${JOB_ID}")
    STATUS=$(echo "$STATUS_RESPONSE" | jq -r '.status' 2>/dev/null)
    
    CURRENT_TIME=$(date +%s)
    ELAPSED=$((CURRENT_TIME - START_TIME))
    
    echo "  [${ELAPSED}s] Status: ${STATUS}"
    
    if [ "$STATUS" = "finished" ] || [ "$STATUS" = "failed" ]; then
        break
    fi
    
    if [ $ELAPSED -gt $MAX_WAIT ]; then
        echo -e "${RED}❌ Timeout (${MAX_WAIT}s dépassé)${NC}"
        exit 1
    fi
    
    sleep $POLL_INTERVAL
done

echo ""
echo -e "${YELLOW}📋 Étape 4: Vérification du résultat${NC}"

RETURNCODE=$(echo "$STATUS_RESPONSE" | jq -r '.returncode' 2>/dev/null)
echo "Return code: ${RETURNCODE}"

if [ "$RETURNCODE" = "0" ]; then
    echo -e "${GREEN}✅ Job terminé avec succès${NC}"
else
    echo -e "${RED}❌ Job échoué (returncode: ${RETURNCODE})${NC}"
    echo ""
    echo "Dernières lignes du log:"
    LOG_RESPONSE=$(curl -s "${BUILDER_URL}/status/${JOB_ID}?tail=20")
    echo "$LOG_RESPONSE" | jq -r '.log_tail' 2>/dev/null || echo "$LOG_RESPONSE"
    exit 1
fi

echo ""
echo -e "${YELLOW}📋 Étape 5: Récupération des dernières lignes du log${NC}"
LOG_RESPONSE=$(curl -s "${BUILDER_URL}/status/${JOB_ID}?tail=10")
echo "$LOG_RESPONSE" | jq -r '.log_tail' 2>/dev/null | head -20 || echo "Pas de log disponible"

echo ""
echo -e "${GREEN}🎉 Tous les tests ont réussi !${NC}"
