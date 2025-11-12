#!/bin/bash
set -e

# Variables
SONAR_HOST="${SONAR_HOST:-https://sonarcloud.io}"
SONAR_TOKEN="${SONAR_TOKEN:-}"
SONAR_ORG="${SONAR_ORG:-}"
SONAR_PROJECT="${SONAR_PROJECT:-}"
PROJECT_ROOT="$(cd "$(git rev-parse --show-toplevel)" && pwd)"
SONAR_SCANNER_VERSION="5.0.1.3006"

echo "🔍 SonarCloud Analysis Script for R-Type"
echo "========================================="

# Vérifier que SonarCloud est accessible
echo "📡 Vérification de la connexion à SonarCloud ($SONAR_HOST)..."
if ! curl -s "$SONAR_HOST/api/system/status" > /dev/null 2>&1; then
    echo "❌ Erreur: SonarCloud n'est pas accessible à $SONAR_HOST"
    echo ""
    echo "💡 Solutions possibles:"
    echo "   1. Vérifiez votre connexion internet"
    echo "   2. Vérifiez votre pare-feu/proxy"
    echo "   3. Essayez d'accéder à https://sonarcloud.io dans votre navigateur"
    exit 1
fi
echo "✅ SonarCloud est accessible"

# Vérifier le token
if [ -z "$SONAR_TOKEN" ]; then
    echo "❌ Erreur: SONAR_TOKEN n'est pas défini"
    echo ""
    echo "💡 Pour générer un token:"
    echo "   1. Allez sur https://sonarcloud.io"
    echo "   2. Connectez-vous avec GitHub/GitLab/Bitbucket"
    echo "   3. Mon compte > Security > Generate Tokens"
    echo "   4. Exportez le token: export SONAR_TOKEN=votre_token"
    echo ""
    exit 1
fi

# Vérifier l'organisation
if [ -z "$SONAR_ORG" ]; then
    echo "❌ Erreur: SONAR_ORG n'est pas défini"
    echo ""
    echo "💡 Pour trouver votre organisation:"
    echo "   1. Allez sur https://sonarcloud.io"
    echo "   2. Vérifiez l'URL: sonarcloud.io/organizations/VOTRE-ORG"
    echo "   3. Exportez: export SONAR_ORG=VOTRE-ORG"
    echo ""
    exit 1
fi

# Vérifier le project key
if [ -z "$SONAR_PROJECT" ]; then
    echo "❌ Erreur: SONAR_PROJECT n'est pas défini"
    echo ""
    echo "💡 Format: organization_repository"
    echo "   Exemple: export SONAR_PROJECT=mon-org_rtype"
    echo ""
    exit 1
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

# Lancer l'analyse SonarCloud
echo "📊 Lancement de l'analyse SonarCloud..."

SONAR_ARGS=(
    "-Dsonar.projectKey=$SONAR_PROJECT"
    "-Dsonar.organization=$SONAR_ORG"
    "-Dsonar.sources=src"
    "-Dsonar.tests=tests"
    "-Dsonar.cfamily.build-wrapper-output=build/bw-output"
    "-Dsonar.host.url=$SONAR_HOST"
    "-Dsonar.login=$SONAR_TOKEN"
)

sonar-scanner "${SONAR_ARGS[@]}"

echo ""
echo "✅ Analyse terminée avec succès!"
echo ""
echo "📈 Consultez les résultats sur:"
echo "   $SONAR_HOST/project/overview?id=$SONAR_PROJECT"
echo ""
echo "💡 Prochaines étapes:"
echo "   1. Ouvrez le dashboard SonarCloud"
echo "   2. Vérifiez les bugs et vulnérabilités"
echo "   3. Corrigez les problèmes détectés"
echo "   4. Re-lancez l'analyse pour vérifier"
