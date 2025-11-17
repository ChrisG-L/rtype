# Builder Docker Scripts

Scripts pour gérer les conteneurs builder sans docker-compose, permettant des builds parallèles sans collision ni rebuild d'image.

## Scripts disponibles

### `build_image.sh`
Construit l'image `rtype-builder:latest` une seule fois.

```bash
./build_image.sh
```

Cette commande doit être exécutée **une seule fois** ou uniquement quand le Dockerfile change.

### `launch_builder.sh`
Lance un conteneur builder avec des paramètres personnalisés.

```bash
./launch_builder.sh [PREFIX] [PORT] [WORKSPACE]
```

**Paramètres:**
- `PREFIX` : Préfixe pour le nom du conteneur et volume (défaut: `local_`)
- `PORT` : Port externe pour l'API (défaut: `8080`)
- `WORKSPACE` : Chemin du workspace (défaut: auto-détecté)

**Exemples:**
```bash
# Lancement simple (local_rtype_builder sur port 8080)
./launch_builder.sh

# Build #1 sur port 8083
./launch_builder.sh build_1_ 8083

# Build #2 sur port 8084
./launch_builder.sh build_2_ 8084
```

**Comportement:**
- ✅ Utilise toujours l'image `rtype-builder:latest` existante (pas de rebuild)
- ✅ Crée le conteneur et le volume s'ils n'existent pas
- ✅ Démarre le conteneur existant s'il est arrêté
- ✅ Affiche un message si déjà en cours d'exécution

### `stop_builder.sh`
Stoppe et supprime un conteneur builder.

```bash
./stop_builder.sh [PREFIX] [REMOVE_VOLUME]
```

**Paramètres:**
- `PREFIX` : Préfixe du conteneur à stopper (défaut: `local_`)
- `REMOVE_VOLUME` : `true`/`yes`/`1` pour supprimer aussi le volume (défaut: `false`)

**Exemples:**
```bash
# Stopper et supprimer le conteneur (garde le volume)
./stop_builder.sh build_1_

# Stopper et supprimer conteneur + volume
./stop_builder.sh build_1_ true
```

## Workflow typique

### Développement local

```bash
# 1. Construire l'image (une seule fois)
./build_image.sh

# 2. Lancer le builder
./launch_builder.sh

# 3. Utiliser l'API
curl http://localhost:8080/health

# 4. Stopper quand terminé
./stop_builder.sh local_ true
```

### Builds parallèles

```bash
# 1. Construire l'image (une seule fois)
./build_image.sh

# 2. Lancer plusieurs builders en parallèle
./launch_builder.sh build_1_ 8083
./launch_builder.sh build_2_ 8084
./launch_builder.sh build_3_ 8085

# 3. Utiliser les APIs sur différents ports
curl http://localhost:8083/health  # Build #1
curl http://localhost:8084/health  # Build #2
curl http://localhost:8085/health  # Build #3

# 4. Nettoyer
./stop_builder.sh build_1_ true
./stop_builder.sh build_2_ true
./stop_builder.sh build_3_ true
```

## Utilisation avec Jenkins

Le Jenkinsfile utilise automatiquement ces scripts :

1. **Setup Build Environment** : Définit `BUILD_PREFIX` et `BUILDER_PORT` uniques
2. **Launch Build Container** : Appelle `launch_builder.sh` avec les paramètres
3. **Build Project** : Utilise l'API sur le port configuré
4. **Post-cleanup** : Appelle `stop_builder.sh` pour nettoyer

## Avantages vs docker-compose

✅ **Pas de rebuild** : L'image est construite une fois et réutilisée  
✅ **Isolation totale** : Chaque build a son conteneur, volume et port  
✅ **Pas de collision** : Les conteneurs ne se recréent pas mutuellement  
✅ **Plus simple** : Pas besoin de gérer les variables d'environnement pour docker-compose  
✅ **Plus rapide** : Pas de temps perdu à vérifier/reconstruire l'image  
✅ **Meilleur contrôle** : Scripts bash faciles à comprendre et déboguer

## Ressources Docker créées

Pour chaque build (exemple avec `PREFIX=build_1_`):
- **Conteneur** : `build_1_rtype_builder`
- **Volume** : `build_1_builder_home`
- **Port** : Mappé selon le paramètre (ex: `8083:8080`)
- **Image** : `rtype-builder:latest` (partagée entre tous)

## Commandes utiles

```bash
# Lister tous les conteneurs builder
docker ps -a --filter "name=rtype_builder"

# Lister tous les volumes builder
docker volume ls --filter "name=builder_home"

# Voir les logs d'un builder
docker logs -f build_1_rtype_builder

# Nettoyer tous les builders arrêtés
docker ps -a --filter "name=rtype_builder" --filter "status=exited" -q | xargs -r docker rm

# Nettoyer tous les volumes builder orphelins
docker volume ls --filter "name=builder_home" -q | xargs -r docker volume rm
```
