#!/bin/bash

set -e

cd "$(git rev-parse --show-toplevel)"

# Launch CI/CD pipeline
echo "Starting CI/CD pipeline..."

cd ci_cd/docker

# Launch Docker docs
echo "Launching Docker documentation..."
docker-compose -f docker-compose.docs.yml up -d

# Launch Docker Jenkins
echo "Launching Docker Jenkins..."
docker-compose -f docker-compose.yml up -d
