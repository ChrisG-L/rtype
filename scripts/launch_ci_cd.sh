#!/usr/bin/env bash

# Colors (ANSI escape codes)
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
GRAY='\033[0;90m'
NC='\033[0m' # No Color

set -euo pipefail

echo -e "${CYAN}=== R-TYPE Infrastructure Deployment with Docker Compose ===${NC}"

# ============================================================================
# Step 1: Check Docker
# ============================================================================
echo -e "\n${YELLOW}[1/5] Checking Docker...${NC}"

if ! command -v docker &> /dev/null; then
    echo -e "${RED}Error: Docker not found!${NC}" >&2
    echo "Please install Docker first" >&2
    exit 1
fi

# Test Docker is running
if ! docker ps &> /dev/null; then
    echo -e "${RED}Error: Docker is not running!${NC}" >&2
    echo "Please start Docker and try again" >&2
    exit 1
fi

echo -e "${GREEN}OK Docker is running${NC}"

# ============================================================================
# Step 2: Check docker-compose
# ============================================================================
echo -e "\n${YELLOW}[2/5] Checking docker-compose...${NC}"

use_compose_v2=false
if docker-compose --version &> /dev/null; then
    compose_version=$(docker-compose --version)
    use_compose_v2=false
elif docker compose version &> /dev/null; then
    compose_version=$(docker compose version)
    use_compose_v2=true
else
    echo -e "${RED}Error: docker-compose not found!${NC}" >&2
    echo "Docker Compose should be included with Docker" >&2
    exit 1
fi

echo -e "${GREEN}OK docker-compose found: ${compose_version}${NC}"

# ============================================================================
# Step 3: Navigate to whanos-infra directory
# ============================================================================
echo -e "\n${YELLOW}[3/5] Navigating to whanos-infra directory...${NC}"

root_dir="$(git rev-parse --show-toplevel)"
infra_dir="$(cd "${root_dir}/ci_cd/docker" && pwd)"

if [[ ! -f "${infra_dir}/docker-compose.yml" ]]; then
    echo -e "${RED}Error: docker-compose.yml for Jenkins not found at ${infra_dir}${NC}" >&2
    exit 1
fi

if [[ ! -f "${infra_dir}/docker-compose.docs.yml" ]]; then
    echo -e "${RED}Error: docker-compose.docs.yml for Documentation not found at ${infra_dir}${NC}" >&2
    exit 1
fi

cd "${infra_dir}"
echo -e "${GREEN}OK Working directory: ${infra_dir}${NC}"

# ============================================================================
# Step 4: Deploy with Docker Compose
# ============================================================================
echo -e "\n${YELLOW}[4/5] Deploying infrastructure with Docker Compose...${NC}"
echo "This will:"
echo -e "${GRAY}  - Build custom Jenkins image (with Docker CLI)${NC}"
echo -e "${GRAY}  - Start Jenkins container (port 8081)${NC}"
echo -e "${GRAY}  - Start MKDocs container (port 8000)${NC}"
echo ""

# Build and start services
if [[ "$use_compose_v2" = true ]]; then
    echo -e "${CYAN}Executing: docker compose -f docker-compose.yml up -d --build${NC}"
    docker compose -f docker-compose.yml up -d --build
else
    echo -e "${CYAN}Executing: docker-compose -f docker-compose.yml up -d --build${NC}"
    docker-compose -f docker-compose.yml up -d --build
fi

compose_exit_code=$?

if [[ $compose_exit_code -ne 0 ]]; then
    echo -e "\n${RED}=== DEPLOYMENT FAILED ===${NC}" >&2
    echo -e "${RED}Docker Compose exited with code: $compose_exit_code${NC}" >&2

    echo -e "\n${YELLOW}Troubleshooting:${NC}" >&2
    echo "1. Check the error messages above" >&2
    echo "2. Verify Docker is running: docker ps" >&2
    echo "3. Check docker-compose.yml syntax" >&2
    if [[ "$use_compose_v2" = true ]]; then
        echo "4. View logs: docker compose logs" >&2
    else
        echo "4. View logs: docker-compose logs" >&2
    fi

    exit $compose_exit_code
fi

if [[ "$use_compose_v2" = true ]]; then
    echo -e "${CYAN}Executing: docker compose -f docker-compose.docs.yml up -d --build${NC}"
    docker compose -f docker-compose.docs.yml up -d --build
else
    echo -e "${CYAN}Executing: docker-compose -f docker-compose.docs.yml up -d --build${NC}"
    docker-compose -f docker-compose.docs.yml up -d --build
fi

compose_exit_code=$?

if [[ $compose_exit_code -ne 0 ]]; then
    echo -e "\n${RED}=== DEPLOYMENT FAILED ===${NC}" >&2
    echo -e "${RED}Docker Compose exited with code: $compose_exit_code${NC}" >&2

    echo -e "\n${YELLOW}Troubleshooting:${NC}" >&2
    echo "1. Check the error messages above" >&2
    echo "2. Verify Docker is running: docker ps" >&2
    echo "3. Check docker-compose.docs.yml syntax" >&2
    if [[ "$use_compose_v2" = true ]]; then
        echo "4. View logs: docker compose logs" >&2
    else
        echo "4. View logs: docker-compose logs" >&2
    fi

    exit $compose_exit_code
fi

# ===========================================================================
# Step 5: Define permission for Docker socket into Host computer
# ===========================================================================

echo -e "\n${YELLOW}[5/5] Setting permissions for Docker socket on host...${NC}"
# Set permissions for Docker socket on host to be readable/writable by all users
sudo chown root:docker /var/run/docker.sock
sudo chmod 660 /var/run/docker.sock

# Print ls -la of /var/run/docker.sock for debugging
echo -e "\n${YELLOW}Docker socket permissions on host:${NC}"
ls -la /var/run/docker.sock

# ============================================================================
# Success!
# ============================================================================
echo -e "\n${GREEN}=== SUCCESS! Infrastructure deployment completed ===${NC}"

echo -e "\n${CYAN}Deployed services:${NC}"
echo "  [OK] Jenkins: http://localhost:8081"
echo "  [OK] Documentation: http://localhost:8000"

echo -e "\n${CYAN}Next steps:${NC}"
echo "1. Access Jenkins:"
echo -e "${GRAY}   Open: http://localhost:8081${NC}"
echo -e "${GRAY}   If needed, get password: docker exec rtype_jenkins cat /var/jenkins_home/secrets/initialAdminPassword${NC}"

echo -e "\n2. Access Documentation:"
echo -e "${GRAY}   Open: http://localhost:8000${NC}"

echo -e "\n3. View logs:"
if [[ "$use_compose_v2" = true ]]; then
    echo -e "${GRAY}   docker compose logs rtype_jenkins      # Jenkins logs${NC}"
    echo -e "${GRAY}   docker compose logs rtype-docs         # Documentation logs${NC}"
else
    echo -e "${GRAY}   docker-compose logs rtype_jenkins      # Jenkins logs${NC}"
    echo -e "${GRAY}   docker-compose logs rtype-docs         # Documentation logs${NC}"
fi

echo -e "\n4. Setup Jenkins (first deployment):"
echo -e "${GRAY}   Read docs/jenkins_setup.md${NC}"
