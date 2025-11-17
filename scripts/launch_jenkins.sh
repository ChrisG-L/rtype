#!/usr/bin/env bash

# Colors (ANSI escape codes)
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
GRAY='\033[0;90m'
NC='\033[0m' # No Color

set -euo pipefail

echo -e "${CYAN}=== Jenkins Deployment with Docker Compose ===${NC}"

# ============================================================================
# Step 1: Check Docker
# ============================================================================
echo -e "\n${YELLOW}[1/5] Checking Docker...${NC}"

if ! command -v docker &> /dev/null; then
    echo -e "${RED}Error: Docker not found!${NC}"
    echo "Please install Docker first"
    exit 1
fi

# Test Docker is running
if ! docker ps &> /dev/null; then
    echo -e "${RED}Error: Docker is not running!${NC}"
    echo "Please start Docker and try again"
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
    echo -e "${RED}Error: docker-compose not found!${NC}"
    echo "Docker Compose should be included with Docker"
    exit 1
fi

echo -e "${GREEN}OK docker-compose found: ${compose_version}${NC}"

# ============================================================================
# Step 3: Navigate to whanos-infra directory
# ============================================================================
echo -e "\n${YELLOW}[3/5] Navigating to whanos-infra directory...${NC}"

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
infra_dir="$(cd "${script_dir}/../.." && pwd)"

if [ ! -f "${infra_dir}/docker-compose.yml" ]; then
    echo -e "${RED}Error: docker-compose.yml not found at ${infra_dir}${NC}"
    exit 1
fi

cd "${infra_dir}"
echo -e "${GREEN}OK Working directory: ${infra_dir}${NC}"

# ============================================================================
# Step 4: Deploy with Docker Compose
# ============================================================================
echo -e "\n${YELLOW}[4/5] Deploying infrastructure with Docker Compose...${NC}"
echo "This will:"
echo -e "${GRAY}  - Build custom Jenkins image (with Docker CLI + Python)${NC}"
echo -e "${GRAY}  - Start Jenkins container (port 8081)${NC}"
echo -e "${GRAY}  - Start Docker Registry (port 5000)${NC}"
echo ""

# Build and start services
if [ "$use_compose_v2" = true ]; then
    echo -e "${CYAN}Executing: docker compose up -d --build${NC}"
    docker compose up -d --build
else
    echo -e "${CYAN}Executing: docker-compose up -d --build${NC}"
    docker-compose up -d --build
fi

compose_exit_code=$?

if [ $compose_exit_code -ne 0 ]; then
    echo -e "\n${RED}=== DEPLOYMENT FAILED ===${NC}"
    echo -e "${RED}Docker Compose exited with code: $compose_exit_code${NC}"

    echo -e "\n${YELLOW}Troubleshooting:${NC}"
    echo "1. Check the error messages above"
    echo "2. Verify Docker is running: docker ps"
    echo "3. Check docker-compose.yml syntax"
    if [ "$use_compose_v2" = true ]; then
        echo "4. View logs: docker compose logs"
    else
        echo "4. View logs: docker-compose logs"
    fi

    exit $compose_exit_code
fi

# ===========================================================================
# Step 5: Define permission for Docker socket Into Jenkins container
# ===========================================================================
echo -e "\n${YELLOW}[5/5] Setting permissions for Docker socket...${NC}"
dockerSocketPath="/var/run/docker.sock"
jenkinsContainerName="jenkins"

# Set permissions for Docker socket inside Jenkins container
docker exec -u 0 $jenkinsContainerName chown -R root:docker $dockerSocketPath
docker exec -u 0 $jenkinsContainerName chmod 660 $dockerSocketPath

# ============================================================================
# Success!
# ============================================================================
echo -e "\n${GREEN}=== SUCCESS! Infrastructure deployment completed ===${NC}"

echo -e "\n${CYAN}Deployed services:${NC}"
echo "  [OK] Jenkins: http://localhost:8081"
echo "  [OK] Docker Registry: http://localhost:5000"

echo -e "\n${CYAN}Next steps:${NC}"
echo "1. Access Jenkins:"
echo -e "${GRAY}   Open: http://localhost:8081${NC}"
echo -e "${GRAY}   Get password: docker exec jenkins cat /var/jenkins_home/secrets/initialAdminPassword${NC}"

echo -e "\n2. Verify deployment:"
echo -e "${GRAY}   docker ps                              # Check containers${NC}"
echo -e "${GRAY}   curl http://localhost:8081             # Test Jenkins${NC}"
echo -e "${GRAY}   curl http://localhost:5000/v2/_catalog # Test Registry${NC}"

echo -e "\n3. View logs:"
if [ "$use_compose_v2" = true ]; then
    echo -e "${GRAY}   docker compose logs jenkins            # Jenkins logs${NC}"
    echo -e "${GRAY}   docker compose logs registry           # Registry logs${NC}"
else
    echo -e "${GRAY}   docker-compose logs jenkins            # Jenkins logs${NC}"
    echo -e "${GRAY}   docker-compose logs registry           # Registry logs${NC}"
fi

echo -e "\n4. Setup Jenkins:"
echo -e "${GRAY}   Read docs/jenkins-setup.md${NC}"

echo -e "\n5. Link a project in Jenkins:"
echo -e "${GRAY}   http://localhost:8081/job/link-project/${NC}"

# ============================================================================
# Prompt: Setup Kubernetes
# ============================================================================
echo -e -n "\n${YELLOW}Would you like to setup Kubernetes now? You should complete the Jenkins Installation Wizard before (y/N): ${NC}"
read -r setup_k8s

if [[ "$setup_k8s" =~ ^[Yy]$ ]]; then
    echo -e "\n${CYAN}=== Starting Kubernetes Setup ===${NC}"
    echo -e "${GRAY}Running: ./scripts/BASH/setup_k8s_for_jenkins.sh${NC}"
    echo ""

    k8s_script_path="${infra_dir}/scripts/BASH/setup_k8s_for_jenkins.sh"

    if [ -f "$k8s_script_path" ]; then
        bash "$k8s_script_path"

        if [ $? -eq 0 ]; then
            echo -e "\n${GREEN}=== Kubernetes setup completed successfully ===${NC}"
        else
            echo -e "\n${RED}=== Kubernetes setup failed ===${NC}"
            echo -e "${YELLOW}You can run it manually later: ./scripts/BASH/setup_k8s_for_jenkins.sh${NC}"
        fi
    else
        echo -e "${RED}Error: Kubernetes setup script not found at $k8s_script_path${NC}"
    fi
else
    echo -e "\n${GRAY}Skipping Kubernetes setup.${NC}"
    echo -e "${GRAY}You can run it later with: ./scripts/BASH/setup_k8s_for_jenkins.sh${NC}"
fi

echo -e "\n${CYAN}Manage services:${NC}"
if [ "$use_compose_v2" = true ]; then
    echo -e "${GRAY}  docker compose stop                    # Stop all services${NC}"
    echo -e "${GRAY}  docker compose start                   # Start all services${NC}"
    echo -e "${GRAY}  docker compose restart                 # Restart all services${NC}"
    echo -e "${GRAY}  docker compose down                    # Stop and remove containers${NC}"
else
    echo -e "${GRAY}  docker-compose stop                    # Stop all services${NC}"
    echo -e "${GRAY}  docker-compose start                   # Start all services${NC}"
    echo -e "${GRAY}  docker-compose restart                 # Restart all services${NC}"
    echo -e "${GRAY}  docker-compose down                    # Stop and remove containers${NC}"
fi

echo -e "\n${GRAY}For more information, see: DOCKER-COMPOSE.md${NC}"
