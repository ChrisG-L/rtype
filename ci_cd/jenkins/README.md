# Jenkins Builder Integration

Ce dossier contient les scripts et configurations pour l'intégration entre Jenkins et le système de build Docker basé sur une API Python.

## Architecture

Le système de build utilise :
- **Docker container** avec un serveur HTTP Python qui exécute les scripts de build/compile de manière asynchrone
- **API REST** pour soumettre des jobs et suivre leur progression
- **Helper Groovy** (`BuilderAPI.groovy`) pour simplifier l'interaction depuis Jenkins

## Fichiers

### `BuilderAPI.groovy`

Classe helper Groovy qui facilite l'interaction avec l'API du builder. Fournit :

- `submitJob(command)` : Soumet un job ('build' ou 'compile') et retourne l'UUID
- `getStatus(jobId, tailLines)` : Récupère le statut d'un job
- `waitForCompletion(jobId, pollInterval, maxWaitTime)` : Attend la completion avec polling
- `runAndWait(command, pollInterval, maxWaitTime)` : Soumet et attend (méthode all-in-one)
- `healthCheck()` : Vérifie que le builder est opérationnel

### Utilisation dans Jenkinsfile

```groovy
script {
    // Charger la librairie
    def builderAPI = load('ci_cd/jenkins/BuilderAPI.groovy')
    def api = new builderAPI.BuilderAPI(this, 'localhost', 8082)
    
    // Vérifier la santé
    if (api.healthCheck()) {
        echo 'Builder opérationnel'
    }
    
    // Soumettre et attendre un job
    def result = api.runAndWait('build', 10, 7200)
    // Poll toutes les 10 secondes, max 2 heures
    
    echo "Job terminé avec returncode: ${result.returncode}"
}
```

## API du Builder

Le serveur Python builder expose les endpoints suivants :

### `GET /` ou `GET /health`
Vérifie l'état du serveur.

**Response:**
```json
{
  "status": "ok",
  "allowed_commands": ["build", "compile"]
}
```

### `POST /run`
Soumet un nouveau job.

**Request:**
```json
{
  "command": "build"  // ou "compile"
}
```

**Response:**
```json
{
  "job_id": "550e8400-e29b-41d4-a716-446655440000"
}
```

### `GET /status/<job_id>?tail=<n>`
Récupère le statut d'un job. Le paramètre `tail` (optionnel) permet de récupérer les n dernières lignes du log.

**Response:**
```json
{
  "status": "finished",  // "queued", "running", "finished", "failed"
  "command": "build",
  "pid": 1234,
  "started_at": 1700000000.0,
  "finished_at": 1700001000.0,
  "returncode": 0,
  "log_tail": "..."  // Si tail param fourni
}
```

## Logs

Les logs de chaque job sont stockés dans :
```
artifacts/builder_jobs/<job_id>.log
```

Ces fichiers persistent même après l'arrêt du conteneur (si le volume est monté).

## Workflow Jenkins

1. **Checkout** : Récupération du code source
2. **Launch Build Container** : Démarrage du conteneur Docker avec le serveur builder
3. **Health Check** : Vérification que le serveur est opérationnel (retry 5x)
4. **Build Project** : Soumission du job via API et attente de completion
5. **Post-actions** : Nettoyage du conteneur

## Variables d'environnement

### Dans Jenkins (Jenkinsfile)
- `BUILD_PREFIX=build_${BUILD_NUMBER}_` : Préfixe unique pour chaque build (permet builds parallèles)
- `BUILDER_PORT=${8082 + BUILD_NUMBER % 1000}` : Port dynamique pour éviter les collisions (commence à 8082)

### Dans docker-compose.build.yml
- `PREFIX` : Préfixe pour les noms de conteneurs et volumes (fourni par Jenkins)
- `BUILDER_PORT` : Port externe mappé (fourni par Jenkins, défaut: 8082)
- `WORKSPACE=/workspace` : Répertoire de travail pour les scripts
- `BUILDER_PORT=8082` (interne) : Port d'écoute du serveur builder dans le conteneur

## Builds parallèles

Le système est conçu pour supporter plusieurs builds simultanés :

### Comment ça marche
1. **Préfixe unique** : Chaque build Jenkins utilise `build_${BUILD_NUMBER}_` comme préfixe
   - Conteneur : `build_123_rtype_builder`, `build_124_rtype_builder`, etc.
   - Volume : `build_123_builder_home`, `build_124_builder_home`, etc.

2. **Port dynamique** : Port calculé avec `8082 + (BUILD_NUMBER % 1000)`
   - Build #1 → port 8083
   - Build #2 → port 8084
   - Build #123 → port 8205

3. **Isolation complète** : Chaque build a son propre conteneur, volume et port

### Exemple de builds parallèles
```bash
# Build #1 (terminal 1)
export PREFIX="build_1_"
export BUILDER_PORT=8083
docker-compose -f ci_cd/docker/docker-compose.build.yml up -d

# Build #2 (terminal 2)
export PREFIX="build_2_"
export BUILDER_PORT=8084
docker-compose -f ci_cd/docker/docker-compose.build.yml up -d

# Les deux builds s'exécutent en parallèle sans conflit
curl http://localhost:8083/health  # Build #1
curl http://localhost:8084/health  # Build #2
```

## Notes

- Le système n'a **pas de timeout** sur l'exécution des scripts (contrairement à l'ancienne version)
- Le polling est configuré à 10 secondes par défaut
- Les logs complets sont disponibles via l'API avec le paramètre `?tail=N`
- En cas d'échec, les 50 dernières lignes du log sont automatiquement affichées dans Jenkins

## Exemple d'utilisation manuelle

```bash
# Démarrer le builder
docker-compose -f ci_cd/docker/docker-compose.build.yml up -d --build

# Vérifier la santé
curl http://localhost:8082/health

# Soumettre un job
JOB_ID=$(curl -s -X POST -H 'Content-Type: application/json' \
  -d '{"command":"build"}' http://localhost:8082/run | jq -r '.job_id')

# Vérifier le statut
curl "http://localhost:8082/status/${JOB_ID}?tail=20"

# Arrêter le builder
docker-compose -f ci_cd/docker/docker-compose.build.yml down
```
