---
tags:
  - configuration
  - client
---

# Configuration Client

## Arguments CLI

```bash
./rtype_client [OPTIONS]
  --server=<host[:port]>  Adresse du serveur (défaut: 127.0.0.1:4125)
  --graphics=<name>       Backend graphique (sfml, sdl2)
  --graphics-path=<path>  Chemin vers un plugin graphique custom
  -h, --help              Afficher l'aide
```

### Exemples

```bash
# Backend SFML (défaut), serveur local
./rtype_client

# Connexion au serveur France (VPS)
./rtype_client --server=51.254.137.175

# Serveur personnalisé avec port custom
./rtype_client --server=myserver.com:4125

# Backend SDL2
./rtype_client --graphics=sdl2

# Plugin personnalisé
./rtype_client --graphics-path=./my_plugin.so
```

---

## Configuration Serveur

### Connexion par défaut

| Service | Port | Protocole |
|---------|------|-----------|
| Auth | 4125 | TCP/TLS |
| Game | 4124 | UDP |
| Voice | 4126 | UDP |

### Configuration in-app

Le client permet de configurer l'adresse du serveur directement depuis l'interface :

- **Écran de connexion** : Appuyez sur ++s++ pour ouvrir la configuration
- **Écran de login** : Cliquez sur le bouton "SERVER" en bas à gauche
- **Auto-affichage** : Après 3 tentatives de connexion échouées

#### Boutons de connexion rapide

| Bouton | Adresse | Description |
|--------|---------|-------------|
| FRANCE | `51.254.137.175` | Serveur VPS France |
| LOCAL | `127.0.0.1` | Serveur local |

### Fichier de configuration

La configuration est persistée automatiquement :

| OS | Chemin |
|----|--------|
| Linux | `~/.config/rtype/rtype_client.json` |
| Windows | `%APPDATA%/RType/rtype_client.json` |

```json
{
    "host": "51.254.137.175",
    "tcpPort": 4125,
    "udpPort": 4124,
    "voicePort": 4126
}
```

!!! tip "Sauvegarde automatique"
    La configuration est sauvegardée automatiquement lors d'une connexion réussie.

---

## Backends Graphiques

Le client utilise un système de plugins pour le rendu graphique.

| Backend | Bibliothèque | Description |
|---------|--------------|-------------|
| `sfml` | `librtype_sfml.so` | Défaut, simple et portable |
| `sdl2` | `librtype_sdl2.so` | Alternative performante |

Les plugins sont cherchés dans :
1. Le dossier courant
2. `./lib/`
3. Le chemin système

---

## Paramètres Utilisateur

Les paramètres sont stockés **côté serveur** (MongoDB) et synchronisés à la connexion.

### Accessibles in-game

| Paramètre | Description | Stockage |
|-----------|-------------|----------|
| Keybindings | 7 actions × 2 touches | MongoDB |
| Ship skin | Skin vaisseau (1-6) | MongoDB |
| Color mode | Mode daltonien | MongoDB |
| Voice settings | PTT/VAD, seuil, gain | MongoDB |
| Audio devices | Micro/speakers préférés | MongoDB |

### Modification

Tous les paramètres se modifient via le menu **Options** in-game.
Ils sont sauvegardés automatiquement sur le serveur.

---

## Résolution et Fullscreen

La fenêtre utilise une résolution logique de 1920×1080 avec support du plein écran.

| Backend | Résolution logique | Mode par défaut |
|---------|-------------------|-----------------|
| SFML | 1920×1080 | Fenêtré |
| SDL2 | 1920×1080 | Fenêtré |

### Basculer en plein écran

Appuyez sur ++f11++ pour basculer entre fenêtré et plein écran desktop (borderless).

### Letterboxing

Le ratio 16:9 est maintenu automatiquement. Sur les écrans non-16:9 :

- Des barres noires apparaissent en haut/bas ou gauche/droite
- Les clics souris sont correctement mappés vers les coordonnées logiques
- Pas de déformation de l'image

!!! info "Plein écran desktop"
    Le mode "desktop fullscreen" (borderless) est utilisé pour des transitions Alt+Tab rapides.

---

## Vérification de Version

Le client vérifie automatiquement sa version à la connexion au serveur.

### Comportement

| Situation | Action |
|-----------|--------|
| Version identique | Connexion normale |
| Version obsolète | Popup avec nombre de commits de retard |
| Version obsolète | Bouton **JENKINS** pour télécharger la mise à jour |

### Popup de mise à jour

Si votre client est obsolète, une popup s'affiche avec :

- Le nombre de commits de retard (ex: "5 commits behind")
- Un bouton **JENKINS** pour accéder aux builds
- Un bouton **CONTINUE** pour continuer quand même (non recommandé)

### Mode Développeur

Pour bypasser la vérification (développement local uniquement) :

```bash
# Créer le fichier à la racine du projet
touch version.dev
```

!!! warning "Sécurité"
    N'utilisez pas `version.dev` en production. Ce fichier est ignoré par git.

### Variable de test

Pour simuler un hash différent (tests) :

```bash
RTYPE_TEST_HASH=abc1234 ./rtype_client
```

### Fichiers associés

| Fichier | Description |
|---------|-------------|
| `src/client/include/core/Version.hpp` | Détection version client |
| `src/server/include/infrastructure/version/VersionHistoryManager.hpp` | Historique serveur |
