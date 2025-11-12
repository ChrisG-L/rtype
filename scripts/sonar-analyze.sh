#!/bin/bash
set -e

# Variables
SONAR_HOST="${SONAR_HOST:-http://localhost:9000}"
SONAR_TOKEN="${SONAR_TOKEN:-}"
PROJECT_ROOT="$(cd "$(git rev-parse --show-toplevel)" && pwd)"
SONAR_SCANNER_VERSION="5.0.1.3006"

echo "🔍 SonarQube Analysis Script for R-Type"
echo "========================================"

# Vérifier que SonarQube est accessible
echo "📡 Vérification de la connexion à SonarQube ($SONAR_HOST)..."
if ! curl -s "$SONAR_HOST/api/system/status" > /dev/null 2>&1; then
    echo "❌ Erreur: SonarQube n'est pas accessible à $SONAR_HOST"
    echo ""
    echo "💡 Solutions possibles:"
    echo "   1. Lancez SonarQube avec:"
    echo "      cd ci_cd/docker && docker-compose -f docker-compose.sonarqube.yml up -d"
    echo ""
    echo "   2. Attendez 1-2 minutes que SonarQube démarre complètement"
    echo ""
    echo "   3. Vérifiez les logs avec:"
    echo "      docker-compose -f ci_cd/docker/docker-compose.sonarqube.yml logs -f sonarqube"
    exit 1
fi
echo "✅ SonarQube est accessible"

# Vérifier le token
if [ -z "$SONAR_TOKEN" ]; then
    echo "⚠️  Avertissement: SONAR_TOKEN n'est pas défini"
    echo ""
    echo "💡 Pour générer un token:"
    echo "   1. Allez sur $SONAR_HOST"
    echo "   2. Connectez-vous (admin/admin par défaut)"
    echo "   3. My Account > Security > Generate Tokens"
    echo "   4. Exportez le token: export SONAR_TOKEN=votre_token"
    echo ""
    read -p "Voulez-vous continuer sans token ? (y/N) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

cd "$PROJECT_ROOT"

# Vérifier que sonar-scanner est installé
if ! command -v sonar-scanner &> /dev/null; then
    echo "📥 Installation de sonar-scanner..."

    SCANNER_DIR="/opt/sonar-scanner-$SONAR_SCANNER_VERSION-linux"

    if [ ! -d "$SCANNER_DIR" ]; then
        echo "   Téléchargement de sonar-scanner $SONAR_SCANNER_VERSION..."
        wget -q "https://binaries.sonarsource.com/Distribution/sonar-scanner-cli/sonar-scanner-cli-$SONAR_SCANNER_VERSION-linux.zip" -O /tmp/sonar-scanner.zip

        echo "   Extraction..."
        sudo unzip -q /tmp/sonar-scanner.zip -d /opt/
        rm /tmp/sonar-scanner.zip
    fi

    export PATH="$PATH:$SCANNER_DIR/bin"
    echo "✅ sonar-scanner installé"
else
    echo "✅ sonar-scanner est déjà installé"
fi

# Vérifier que build-wrapper est installé (pour C++)
if ! command -v build-wrapper-linux-x86-64 &> /dev/null; then
    echo "📥 Installation de build-wrapper pour C++..."

    if [ ! -d "/opt/build-wrapper-linux-x86" ]; then
        echo "   Téléchargement du build-wrapper..."
        wget -q "https://sonarcloud.io/static/cpp/build-wrapper-linux-x86.zip" -O /tmp/build-wrapper.zip

        echo "   Extraction..."
        sudo unzip -q /tmp/build-wrapper.zip -d /opt/
        rm /tmp/build-wrapper.zip
    fi

    export PATH="$PATH:/opt/build-wrapper-linux-x86"
    echo "✅ build-wrapper installé"
else
    echo "✅ build-wrapper est déjà installé"
fi

# Vérifier que le projet est compilé
if [ ! -d "build" ]; then
    echo "🔨 Projet non compilé, lancement du build..."
    ./scripts/build.sh
fi

# Nettoyer les anciens résultats du build-wrapper
echo "🧹 Nettoyage des anciens résultats..."
rm -rf build/bw-output

# Compiler avec build-wrapper
echo "🔨 Compilation avec build-wrapper..."
build-wrapper-linux-x86-64 --out-dir build/bw-output ./scripts/compile.sh

# Lancer l'analyse SonarQube
echo "📊 Lancement de l'analyse SonarQube..."

SONAR_ARGS=(
    "-Dsonar.projectKey=rtype"
    "-Dsonar.sources=src"
    "-Dsonar.tests=tests"
    "-Dsonar.cfamily.build-wrapper-output=build/bw-output"
    "-Dsonar.host.url=$SONAR_HOST"
)

if [ -n "$SONAR_TOKEN" ]; then
    SONAR_ARGS+=("-Dsonar.login=$SONAR_TOKEN")
fi

sonar-scanner "${SONAR_ARGS[@]}"

echo ""
echo "✅ Analyse terminée avec succès!"
echo ""
echo "📈 Consultez les résultats sur:"
echo "   $SONAR_HOST/dashboard?id=rtype"
echo ""
echo "💡 Prochaines étapes:"
echo "   1. Ouvrez le dashboard SonarQube"
echo "   2. Vérifiez les bugs et vulnérabilités"
echo "   3. Corrigez les problèmes détectés"
echo "   4. Re-lancez l'analyse pour vérifier"
