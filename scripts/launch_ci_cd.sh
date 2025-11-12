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

echo ""
echo "✅ CI/CD Infrastructure is ready!"
echo ""
echo "📋 Available Services:"
echo "   • Documentation: http://localhost:8000"
echo "   • Jenkins:       http://localhost:8080"
echo ""
echo "🛑 To stop all services:"
echo "   cd ci_cd/docker"
echo "   docker-compose -f docker-compose.yml down"
echo "   docker-compose -f docker-compose.docs.yml down"
