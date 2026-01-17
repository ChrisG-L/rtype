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

## Résolution

La fenêtre utilise une résolution fixe définie dans le backend graphique.

| Backend | Résolution | Mode |
|---------|------------|------|
| SFML | 1920×1080 | Fenêtré |
| SDL2 | 1920×1080 | Fenêtré |
