---
tags:
  - configuration
  - client
---

# Configuration Client

## Arguments CLI

```bash
./rtype_client [OPTIONS]
  --graphics=<name>       Backend graphique (sfml, sdl2)
  --graphics-path=<path>  Chemin vers un plugin graphique custom
  -h, --help              Afficher l'aide
```

### Exemples

```bash
# Backend SFML (défaut)
./rtype_client

# Backend SDL2
./rtype_client --graphics=sdl2

# Plugin personnalisé
./rtype_client --graphics-path=./my_plugin.so
```

---

## Connexion Serveur

Le client se connecte automatiquement à `127.0.0.1` sur les ports :

| Service | Port | Protocole |
|---------|------|-----------|
| Auth | 4125 | TCP/TLS |
| Game | 4124 | UDP |
| Voice | 4126 | UDP |

!!! note "Configuration réseau"
    L'adresse du serveur est actuellement codée en dur (`Boot.cpp`).
    Pour se connecter à un autre serveur, il faut modifier le code source.

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
