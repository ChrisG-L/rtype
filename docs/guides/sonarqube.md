# SonarQube - Analyse de Qualité de Code

## Qu'est-ce que SonarQube ?

**SonarQube** est une plateforme open-source d'inspection continue de la qualité du code. Elle effectue des analyses automatiques pour détecter :

- **Bugs** : Erreurs de code qui peuvent causer des problèmes en production
- **Vulnérabilités** : Failles de sécurité potentielles (injection SQL, XSS, etc.)
- **Code Smells** : Mauvaises pratiques qui rendent le code difficile à maintenir
- **Duplication de code** : Code répété qui devrait être refactorisé
- **Couverture de tests** : Pourcentage de code couvert par les tests
- **Complexité** : Mesure de la complexité cyclomatique du code

!!! info "Pourquoi utiliser SonarQube ?"
    - **Qualité** : Maintenir un code propre et maintenable
    - **Sécurité** : Détecter les vulnérabilités avant la production
    - **Dette technique** : Suivre et réduire la dette technique
    - **Standards** : Respecter les bonnes pratiques C++23

## SonarQube dans le projet R-Type

Dans ce projet, SonarQube vous permet de :

1. **Analyser le code C++23** pour détecter les problèmes de qualité
2. **Suivre la couverture de tests** (intégration avec Google Test)
3. **Détecter les vulnérabilités de sécurité** (buffer overflows, injections, etc.)
4. **Mesurer la complexité** du code (fonctions trop complexes, etc.)
5. **Visualiser la dette technique** et planifier les refactorings
6. **Générer des rapports** pour l'équipe et les reviews de code

## Installation et Configuration

### Utiliser SonarCloud (Recommandé)

Ce projet utilise **SonarCloud**, la version cloud de SonarQube hébergée en ligne. Aucune installation locale n'est nécessaire.

**Avantages de SonarCloud** :
- ✅ Pas d'infrastructure à maintenir
- ✅ Toujours à jour avec la dernière version
- ✅ Gratuit pour les projets open-source
- ✅ Intégration facile avec GitHub/GitLab
- ✅ Analyses illimitées

### Accès à l'interface web

**URL** : https://sonarcloud.io

!!! info "Authentification"
    Connectez-vous avec votre compte GitHub, GitLab ou Bitbucket. Aucun identifiant spécifique n'est requis.

## Configuration du Projet

### Étape 1 : Créer un projet dans SonarCloud

1. Connectez-vous à https://sonarcloud.io
2. Cliquez sur **"+"** puis **"Analyze new project"**
3. Sélectionnez votre organisation GitHub/GitLab
4. Choisissez le repository **rtype**
5. Configurez les informations :
   - **Project key** : `votre-org_rtype`
   - **Display name** : `R-Type Game`
6. Cliquez sur **"Set Up"**

### Étape 2 : Générer un token d'authentification

1. Dans le projet, cliquez sur **"With other CI tools"** ou **"Locally"**
2. Générez un token :
   - **Name** : `rtype-ci-analysis`
   - Copiez le token généré (vous ne pourrez plus le voir après)
3. Conservez ce token de manière sécurisée (ex: GitHub Secrets pour CI/CD)

### Étape 3 : Configuration du scanner

Créez un fichier `sonar-project.properties` à la racine du projet :

```properties
# Informations du projet (utilisez votre organization key de SonarCloud)
sonar.projectKey=votre-org_rtype
sonar.organization=votre-org
sonar.projectName=R-Type Game
sonar.projectVersion=1.0

# Chemins du code source
sonar.sources=src
sonar.tests=tests

# Exclusions
sonar.exclusions=**/third_party/**,**/build/**,**/artifacts/**,**/*.pb.cc,**/*.pb.h

# Langage et encodage
sonar.language=c++
sonar.sourceEncoding=UTF-8

# Build wrapper (pour C++)
sonar.cfamily.build-wrapper-output=build/bw-output

# Couverture de tests
sonar.cfamily.gcov.reportsPath=build/coverage
sonar.coverageReportPaths=build/coverage/coverage.xml

# Standards C++
sonar.cfamily.standard=c++23
sonar.cfamily.threads=4

# URL de SonarCloud
sonar.host.url=https://sonarcloud.io
```

## Analyse du Code

### Méthode 1 : Analyse locale avec sonar-scanner

#### Installation du scanner

```bash
# Télécharger sonar-scanner
wget https://binaries.sonarsource.com/Distribution/sonar-scanner-cli/sonar-scanner-cli-5.0.1.3006-linux.zip

# Extraire
unzip sonar-scanner-cli-5.0.1.3006-linux.zip -d /opt/

# Ajouter au PATH
echo 'export PATH=$PATH:/opt/sonar-scanner-5.0.1.3006-linux/bin' >> ~/.bashrc
source ~/.bashrc
```

#### Lancer l'analyse

```bash
# Depuis la racine du projet
sonar-scanner \
  -Dsonar.projectKey=votre-org_rtype \
  -Dsonar.organization=votre-org \
  -Dsonar.sources=src \
  -Dsonar.tests=tests \
  -Dsonar.host.url=https://sonarcloud.io \
  -Dsonar.login=VOTRE_TOKEN_ICI
```

!!! tip "Build Wrapper pour C++"
    Pour une analyse complète du code C++, utilisez le build-wrapper de SonarQube qui capture les informations de compilation.

### Méthode 2 : Analyse avec Build Wrapper (Recommandé pour C++)

Le build wrapper capture les informations de compilation pour une analyse plus précise :

```bash
# 1. Télécharger le build wrapper
wget https://sonarcloud.io/static/cpp/build-wrapper-linux-x86.zip
unzip build-wrapper-linux-x86.zip -d /opt/

# 2. Ajouter au PATH
export PATH=$PATH:/opt/build-wrapper-linux-x86

# 3. Nettoyer le build
rm -rf build
./scripts/build.sh

# 4. Compiler avec le build wrapper
build-wrapper-linux-x86-64 --out-dir build/bw-output ./scripts/compile.sh

# 5. Lancer l'analyse SonarCloud
sonar-scanner \
  -Dsonar.projectKey=votre-org_rtype \
  -Dsonar.organization=votre-org \
  -Dsonar.sources=src \
  -Dsonar.tests=tests \
  -Dsonar.cfamily.build-wrapper-output=build/bw-output \
  -Dsonar.host.url=https://sonarcloud.io \
  -Dsonar.login=VOTRE_TOKEN_ICI
```

### Méthode 3 : Script d'analyse automatique

Le projet inclut déjà un script `scripts/sonar-analyze.sh` qui automatise l'analyse :

```bash
#!/bin/bash
set -e

# Variables (configurez votre organisation et token)
SONAR_HOST="${SONAR_HOST:-https://sonarcloud.io}"
SONAR_TOKEN="${SONAR_TOKEN:-your-token-here}"
SONAR_ORG="${SONAR_ORG:-votre-org}"
SONAR_PROJECT="${SONAR_PROJECT:-votre-org_rtype}"
PROJECT_ROOT="$(cd "$(git rev-parse --show-toplevel)" && pwd)"

echo "🔍 Lancement de l'analyse SonarCloud..."

# Vérifier que SonarCloud est accessible
if ! curl -s "$SONAR_HOST/api/system/status" > /dev/null; then
    echo "❌ Erreur: SonarCloud n'est pas accessible à $SONAR_HOST"
    echo "💡 Vérifiez votre connexion internet"
    exit 1
fi

cd "$PROJECT_ROOT"

# Nettoyer et recompiler avec build-wrapper
echo "🧹 Nettoyage du build..."
rm -rf build/bw-output

echo "🔨 Compilation avec build-wrapper..."
build-wrapper-linux-x86-64 --out-dir build/bw-output ./scripts/compile.sh

# Lancer l'analyse
echo "📊 Analyse SonarCloud en cours..."
sonar-scanner \
  -Dsonar.projectKey="$SONAR_PROJECT" \
  -Dsonar.organization="$SONAR_ORG" \
  -Dsonar.sources=src \
  -Dsonar.tests=tests \
  -Dsonar.cfamily.build-wrapper-output=build/bw-output \
  -Dsonar.host.url="$SONAR_HOST" \
  -Dsonar.login="$SONAR_TOKEN"

echo "✅ Analyse terminée!"
echo "📈 Consultez les résultats sur: $SONAR_HOST/project/overview?id=$SONAR_PROJECT"
```

Puis lancez-le avec vos variables d'environnement :

```bash
# Configurer les variables d'environnement
export SONAR_TOKEN="votre-token"
export SONAR_ORG="votre-org"
export SONAR_PROJECT="votre-org_rtype"

# Lancer l'analyse
./scripts/sonar-analyze.sh
```

## Intégration avec Jenkins

Pour analyser automatiquement le code à chaque push, ajoutez une étape dans le `Jenkinsfile` :

```groovy
stage('SonarQube Analysis') {
    steps {
        script {
            // Installer le scanner si nécessaire
            sh '''
                if [ ! -d "/opt/sonar-scanner" ]; then
                    wget https://binaries.sonarsource.com/Distribution/sonar-scanner-cli/sonar-scanner-cli-5.0.1.3006-linux.zip
                    unzip sonar-scanner-cli-5.0.1.3006-linux.zip -d /opt/
                    mv /opt/sonar-scanner-* /opt/sonar-scanner
                fi
            '''

            // Lancer l'analyse
            withSonarQubeEnv('SonarQube') {
                sh '''
                    /opt/sonar-scanner/bin/sonar-scanner \
                        -Dsonar.projectKey=rtype \
                        -Dsonar.sources=src \
                        -Dsonar.tests=tests \
                        -Dsonar.cfamily.build-wrapper-output=build/bw-output
                '''
            }
        }
    }
}

stage('Quality Gate') {
    steps {
        timeout(time: 5, unit: 'MINUTES') {
            waitForQualityGate abortPipeline: true
        }
    }
}
```

## Interpréter les Résultats

### Dashboard principal

Le dashboard SonarQube affiche plusieurs métriques clés :

#### 1. Bugs
**Définition** : Erreurs de code qui peuvent causer des problèmes en production

**Exemples en C++** :
- Déréférencement de pointeur null
- Division par zéro
- Accès hors limites de tableau
- Fuite mémoire

**Priorité** : 🔴 **CRITIQUE** - À corriger immédiatement

#### 2. Vulnérabilités
**Définition** : Failles de sécurité exploitables

**Exemples en C++** :
- Buffer overflow
- Injection de commandes
- Utilisation de fonctions non sécurisées (`strcpy`, `sprintf`)
- Mauvaise gestion des ressources

**Priorité** : 🔴 **CRITIQUE** - À corriger immédiatement

#### 3. Code Smells
**Définition** : Mauvaises pratiques qui rendent le code difficile à maintenir

**Exemples en C++** :
- Fonctions trop longues (>100 lignes)
- Complexité cyclomatique élevée
- Duplication de code
- Mauvais nommage de variables
- Commentaires TODO non résolus

**Priorité** : 🟡 **MOYEN** - À corriger progressivement

#### 4. Couverture de tests
**Définition** : Pourcentage de code couvert par les tests unitaires

**Objectifs** :
- ✅ **>80%** : Très bonne couverture
- 🟡 **60-80%** : Couverture acceptable
- 🔴 **<60%** : Couverture insuffisante

#### 5. Duplication
**Définition** : Pourcentage de code dupliqué

**Objectifs** :
- ✅ **<3%** : Très bon
- 🟡 **3-5%** : Acceptable
- 🔴 **>5%** : Refactoring nécessaire

### Niveaux de sévérité

| Sévérité | Description | Action |
|----------|-------------|--------|
| 🔴 **Blocker** | Bloque le déploiement | Corriger immédiatement |
| 🟠 **Critical** | Problème majeur | Corriger avant merge |
| 🟡 **Major** | Problème important | Corriger rapidement |
| 🔵 **Minor** | Problème mineur | Corriger progressivement |
| ⚪ **Info** | Information | Optionnel |

## Quality Gates

Les Quality Gates sont des seuils de qualité que le code doit respecter :

### Configuration recommandée pour R-Type

```yaml
Quality Gate: "R-Type Standards"

Conditions:
  - Coverage on New Code >= 80%
  - Duplicated Lines on New Code <= 3%
  - Maintainability Rating on New Code = A
  - Reliability Rating on New Code = A
  - Security Rating on New Code = A
  - Security Hotspots Reviewed = 100%
```

### Configurer un Quality Gate

1. Allez dans **Quality Gates** > **Create**
2. Nommez-le `R-Type Standards`
3. Ajoutez les conditions ci-dessus
4. Associez-le au projet R-Type

## Workflow de Développement avec SonarQube

### Avant de créer une Pull Request

```bash
# 1. Configurer les variables d'environnement
export SONAR_TOKEN="votre-token"
export SONAR_ORG="votre-org"
export SONAR_PROJECT="votre-org_rtype"

# 2. Lancer l'analyse locale
./scripts/sonar-analyze.sh

# 3. Consulter les résultats
# https://sonarcloud.io/project/overview?id=votre-org_rtype

# 4. Corriger les problèmes détectés

# 5. Re-analyser pour vérifier
./scripts/sonar-analyze.sh

# 6. Si tout est vert, créer la PR
```

### Règles à suivre

!!! success "Checklist avant merge"
    - [ ] Aucun bug détecté
    - [ ] Aucune vulnérabilité détectée
    - [ ] Code Smells < 10 pour les nouveaux fichiers
    - [ ] Couverture de tests >= 80% pour le nouveau code
    - [ ] Duplication < 3%
    - [ ] Quality Gate PASSED

## Commandes Utiles

| Commande | Description |
|----------|-------------|
| `export SONAR_TOKEN="token"` | Configurer le token d'authentification |
| `export SONAR_ORG="org"` | Configurer l'organisation SonarCloud |
| `export SONAR_PROJECT="org_rtype"` | Configurer le projet key |
| `./scripts/sonar-analyze.sh` | Lancer une analyse complète |
| `sonar-scanner` | Lancer une analyse manuelle |
| `build-wrapper-linux-x86-64 --out-dir build/bw-output ./scripts/compile.sh` | Compiler avec build-wrapper |

## Résolution des Problèmes

### Erreur "Unauthorized" lors de l'analyse

```bash
# Vérifier que votre token est valide
# 1. Allez sur https://sonarcloud.io
# 2. Mon compte > Security > Tokens
# 3. Générez un nouveau token si nécessaire
# 4. Configurez-le: export SONAR_TOKEN="nouveau-token"
```

### Erreur "Organization not found"

```bash
# Vérifier le nom de votre organisation sur SonarCloud
# 1. Allez sur https://sonarcloud.io
# 2. Vérifiez le nom dans l'URL : sonarcloud.io/organizations/VOTRE-ORG
# 3. Configurez: export SONAR_ORG="VOTRE-ORG"
```

### Analyse échoue avec "build-wrapper not found"

```bash
# Installer le build-wrapper
wget https://sonarcloud.io/static/cpp/build-wrapper-linux-x86.zip
unzip build-wrapper-linux-x86.zip -d /opt/
export PATH=$PATH:/opt/build-wrapper-linux-x86
```

### Erreur "Project key already exists"

```bash
# Le projet existe déjà sur SonarCloud
# 1. Utilisez le project key existant dans sonar-project.properties
# 2. Ou supprimez le projet sur SonarCloud et recréez-le
```

### Problème de connexion à SonarCloud

```bash
# Vérifier la connectivité
curl -I https://sonarcloud.io

# Si le problème persiste, vérifier votre pare-feu/proxy
```

## Ressources Additionnelles

- [Documentation officielle SonarCloud](https://docs.sonarcloud.io/)
- [Documentation SonarQube](https://docs.sonarqube.org/)
- [Règles C++ SonarQube](https://rules.sonarsource.com/cpp/)
- [Build Wrapper pour C++](https://docs.sonarqube.org/latest/analyzing-source-code/languages/c-family/)
- [Quality Gates](https://docs.sonarcloud.io/improving/quality-gates/)
- [Intégration CI/CD](https://docs.sonarcloud.io/advanced-setup/ci-based-analysis/)

## Prochaines Étapes

1. [Créer un compte SonarCloud](#installation-et-configuration)
2. [Configurer votre projet](#configuration-du-projet)
3. [Lancer votre première analyse](#analyse-du-code)
4. [Configurer les Quality Gates](#quality-gates)
5. [Intégrer à Jenkins](#integration-avec-jenkins)
6. [Consulter la [FAQ](../reference/faq.md) pour plus de questions]

---

**Note** : SonarCloud est un outil puissant pour maintenir la qualité du code. Utilisez-le régulièrement pour détecter les problèmes tôt et maintenir un code propre et maintenable !
