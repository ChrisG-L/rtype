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

## Installation et Lancement

### Méthode 1 : Lancement avec Docker (Recommandé)

SonarQube est disponible via Docker Compose :

```bash
# Depuis la racine du projet
cd ci_cd/docker

# Lancer SonarQube
docker-compose -f docker-compose.sonarqube.yml up -d

# Vérifier que SonarQube est démarré
docker-compose -f docker-compose.sonarqube.yml ps
```

!!! warning "Temps de démarrage"
    SonarQube peut prendre 1-2 minutes pour démarrer complètement. Attendez que les logs indiquent "SonarQube is up".

### Accès à l'interface web

Une fois lancé, SonarQube est accessible à :

**URL** : http://localhost:9000

**Identifiants par défaut** :
- **Username** : `admin`
- **Password** : `admin`

!!! danger "Première connexion"
    Lors de votre première connexion, SonarQube vous demandera de changer le mot de passe par défaut. **Faites-le immédiatement** pour sécuriser l'instance.

### Arrêter SonarQube

```bash
cd ci_cd/docker
docker-compose -f docker-compose.sonarqube.yml down
```

## Configuration du Projet

### Étape 1 : Créer un projet dans SonarQube

1. Connectez-vous à http://localhost:9000
2. Cliquez sur **"Create new project"**
3. Remplissez les informations :
   - **Project key** : `rtype`
   - **Display name** : `R-Type Game`
4. Cliquez sur **"Set Up"**

### Étape 2 : Générer un token d'authentification

1. Dans le projet, cliquez sur **"Locally"**
2. Générez un token :
   - **Name** : `rtype-local-analysis`
   - Copiez le token généré (vous ne pourrez plus le voir après)
3. Conservez ce token de manière sécurisée

### Étape 3 : Configuration du scanner

Créez un fichier `sonar-project.properties` à la racine du projet :

```properties
# Informations du projet
sonar.projectKey=rtype
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
  -Dsonar.projectKey=rtype \
  -Dsonar.sources=src \
  -Dsonar.tests=tests \
  -Dsonar.host.url=http://localhost:9000 \
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

# 5. Lancer l'analyse SonarQube
sonar-scanner \
  -Dsonar.projectKey=rtype \
  -Dsonar.sources=src \
  -Dsonar.tests=tests \
  -Dsonar.cfamily.build-wrapper-output=build/bw-output \
  -Dsonar.host.url=http://localhost:9000 \
  -Dsonar.login=VOTRE_TOKEN_ICI
```

### Méthode 3 : Script d'analyse automatique

Créez un script `scripts/sonar-analyze.sh` :

```bash
#!/bin/bash
set -e

# Variables
SONAR_HOST="http://localhost:9000"
SONAR_TOKEN="${SONAR_TOKEN:-your-token-here}"
PROJECT_ROOT="$(cd "$(git rev-parse --show-toplevel)" && pwd)"

echo "🔍 Lancement de l'analyse SonarQube..."

# Vérifier que SonarQube est accessible
if ! curl -s "$SONAR_HOST/api/system/status" > /dev/null; then
    echo "❌ Erreur: SonarQube n'est pas accessible à $SONAR_HOST"
    echo "💡 Lancez-le avec: cd ci_cd/docker && docker-compose -f docker-compose.sonarqube.yml up -d"
    exit 1
fi

cd "$PROJECT_ROOT"

# Nettoyer et recompiler avec build-wrapper
echo "🧹 Nettoyage du build..."
rm -rf build/bw-output

echo "🔨 Compilation avec build-wrapper..."
build-wrapper-linux-x86-64 --out-dir build/bw-output ./scripts/compile.sh

# Lancer l'analyse
echo "📊 Analyse SonarQube en cours..."
sonar-scanner \
  -Dsonar.projectKey=rtype \
  -Dsonar.sources=src \
  -Dsonar.tests=tests \
  -Dsonar.cfamily.build-wrapper-output=build/bw-output \
  -Dsonar.host.url="$SONAR_HOST" \
  -Dsonar.login="$SONAR_TOKEN"

echo "✅ Analyse terminée!"
echo "📈 Consultez les résultats sur: $SONAR_HOST/dashboard?id=rtype"
```

Puis lancez-le :

```bash
chmod +x scripts/sonar-analyze.sh
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
# 1. Lancer l'analyse locale
./scripts/sonar-analyze.sh

# 2. Consulter les résultats
# http://localhost:9000/dashboard?id=rtype

# 3. Corriger les problèmes détectés

# 4. Re-analyser pour vérifier
./scripts/sonar-analyze.sh

# 5. Si tout est vert, créer la PR
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
| `docker-compose -f ci_cd/docker/docker-compose.sonarqube.yml up -d` | Lancer SonarQube |
| `docker-compose -f ci_cd/docker/docker-compose.sonarqube.yml down` | Arrêter SonarQube |
| `docker-compose -f ci_cd/docker/docker-compose.sonarqube.yml logs -f` | Voir les logs SonarQube |
| `./scripts/sonar-analyze.sh` | Lancer une analyse complète |
| `sonar-scanner` | Lancer une analyse manuelle |
| `build-wrapper-linux-x86-64 --out-dir build/bw-output ./scripts/compile.sh` | Compiler avec build-wrapper |

## Résolution des Problèmes

### SonarQube ne démarre pas

```bash
# Vérifier les logs
docker-compose -f ci_cd/docker/docker-compose.sonarqube.yml logs

# Vérifier l'espace disque (SonarQube nécessite au moins 2GB)
df -h

# Redémarrer complètement
docker-compose -f ci_cd/docker/docker-compose.sonarqube.yml down -v
docker-compose -f ci_cd/docker/docker-compose.sonarqube.yml up -d
```

### Erreur "Elasticsearch: max virtual memory areas too low"

```bash
# Sur Linux
sudo sysctl -w vm.max_map_count=262144

# Pour rendre permanent
echo "vm.max_map_count=262144" | sudo tee -a /etc/sysctl.conf
```

### Analyse échoue avec "build-wrapper not found"

```bash
# Installer le build-wrapper
wget https://sonarcloud.io/static/cpp/build-wrapper-linux-x86.zip
unzip build-wrapper-linux-x86.zip -d /opt/
export PATH=$PATH:/opt/build-wrapper-linux-x86
```

### Token d'authentification invalide

```bash
# Générer un nouveau token :
# 1. Allez sur http://localhost:9000
# 2. My Account > Security > Generate Tokens
# 3. Utilisez le nouveau token dans vos commandes
```

## Ressources Additionnelles

- [Documentation officielle SonarQube](https://docs.sonarqube.org/)
- [Règles C++ SonarQube](https://rules.sonarsource.com/cpp/)
- [Build Wrapper pour C++](https://docs.sonarqube.org/latest/analyzing-source-code/languages/c-family/)
- [Quality Gates](https://docs.sonarqube.org/latest/user-guide/quality-gates/)
- [Intégration Jenkins](https://docs.sonarqube.org/latest/analyzing-source-code/ci-integration/jenkins-integration/)

## Prochaines Étapes

1. [Installer et configurer SonarQube](#installation-et-lancement)
2. [Créer votre premier projet](#configuration-du-projet)
3. [Lancer votre première analyse](#analyse-du-code)
4. [Configurer les Quality Gates](#quality-gates)
5. [Intégrer à Jenkins](#integration-avec-jenkins)
6. [Consulter la [FAQ](../reference/faq.md) pour plus de questions]

---

**Note** : SonarQube est un outil puissant pour maintenir la qualité du code. Utilisez-le régulièrement pour détecter les problèmes tôt et maintenir un code propre et maintenable !
