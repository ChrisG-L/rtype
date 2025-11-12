#!/bin/bash

set -e

cd "$(git rev-parse --show-toplevel)"

# Launch CI/CD pipeline
echo "🚀 Starting CI/CD Infrastructure..."
echo "===================================="

cd ci_cd/docker

# Launch Docker docs
echo ""
echo "📚 Launching Documentation (MkDocs)..."
docker-compose -f docker-compose.docs.yml up -d
echo "   ✅ Documentation available at: http://localhost:8000"

# Launch Docker Jenkins
echo ""
echo "🔧 Launching Jenkins CI/CD..."
docker-compose -f docker-compose.yml up -d
echo "   ✅ Jenkins available at: http://localhost:8080"

# Ask user if they want to launch SonarQube
echo ""
echo "🔍 SonarQube - Code Quality Analysis"
read -p "   Do you want to launch SonarQube? (y/N) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo "   Launching SonarQube..."
    docker-compose -f docker-compose.sonarqube.yml up -d
    echo "   ✅ SonarQube available at: http://localhost:9000"
    echo "   ⚠️  SonarQube may take 1-2 minutes to fully start"
    echo "   💡 Default credentials: admin/admin (change on first login)"
else
    echo "   ⏭️  Skipping SonarQube (launch it later with: cd ci_cd/docker && docker-compose -f docker-compose.sonarqube.yml up -d)"
fi

echo ""
echo "✅ CI/CD Infrastructure is ready!"
echo ""
echo "📋 Available Services:"
echo "   • Documentation: http://localhost:8000"
echo "   • Jenkins:       http://localhost:8080"
if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo "   • SonarQube:     http://localhost:9000"
fi
echo ""
echo "🛑 To stop all services:"
echo "   cd ci_cd/docker"
echo "   docker-compose -f docker-compose.yml down"
echo "   docker-compose -f docker-compose.docs.yml down"
if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo "   docker-compose -f docker-compose.sonarqube.yml down"
fi
