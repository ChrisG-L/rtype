#!/bin/bash
# Entrypoint pour le builder permanent
# Lance rsync daemon (pour uploads) + API HTTP Python (pour builds)

set -e

echo "🚀 Démarrage du Builder"
echo "======================="

# Créer le répertoire pour les workspaces
mkdir -p /workspace/builds
echo "📁 Répertoire builds créé: /workspace/builds"

# Lancer rsync daemon en arrière-plan
echo "🔄 Démarrage du rsync daemon..."
rsync --daemon --config=/etc/rsyncd.conf
echo "✅ Rsync daemon lancé sur port 873"

# Afficher les infos de configuration
echo ""
echo "Configuration:"
echo "  - API HTTP:    port 8080"
echo "  - Rsync:       port 873"
echo "  - Workspace:   /workspace/builds"
echo ""

# Lancer le serveur Python API (en foreground)
echo "🐍 Démarrage de l'API Python..."
exec python3 /workspace/ci_cd/docker/builder/main.py
