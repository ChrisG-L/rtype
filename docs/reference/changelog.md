---
tags:
  - reference
  - changelog
---

# Changelog

Historique des versions de R-Type.

Le format est basé sur [Keep a Changelog](https://keepachangelog.com/fr/1.0.0/),
et ce projet adhère au [Versionnement Sémantique](https://semver.org/lang/fr/).

---

## [Unreleased]

### Added

#### Gameplay
- **Mode Fullscreen** : Touche ++f11++ pour basculer entre fenêtré (1920x1080) et plein écran
- **Letterboxing automatique** : Maintien du ratio 16:9 avec barres noires sur écrans non-16:9
- **Mode Solo** : Possibilité de jouer seul (1 joueur minimum au lieu de 2)
- **Affichage des vrais pseudos** : Les autres joueurs affichent leur vrai pseudo, "You" uniquement pour le joueur local

#### Leaderboard
- Affichage du rang global en temps réel pendant la partie
- Affichage du meilleur score personnel avec indicateur "NEW!"
- Mise à jour périodique du rang (toutes les 10 secondes)
- Tri par colonnes cliquables dans le leaderboard (Score, Wave, Kills, Time)
- **Filtrage par mode de jeu** : Solo, Duo, Trio, 4P, 5P, 6P ou tous les modes
- Colonne "MODE" affichée quand le filtre "ALL" est sélectionné

#### Système
- **Vérification de version automatique** : Comparaison du hash Git client/serveur à la connexion
- **Historique des versions** : Affiche le nombre de commits de retard si version obsolète
- **Bouton Jenkins** : Accès direct au téléchargement si mise à jour nécessaire
- **Mode développeur** : Fichier `version.dev` pour bypasser la vérification
- **Script run-client.sh** : Wrapper Linux pour le support audio PipeWire/JACK (voice chat)

#### Discord Bots
- **Bot Admin** : Administration à distance via Discord (slash commands)
  - Commandes : `/status`, `/sessions`, `/rooms`, `/kick`, `/ban`, `/broadcast`
  - Connexion sécurisée via TCPAdminServer (port 4127, token 256-bit)
- **Bot Leaderboard** : Statistiques et classements via Discord
  - Commandes : `/leaderboard`, `/stats`, `/achievements`, `/history`, `/compare`
  - Pagination avec boutons, autocomplétion des joueurs
  - **Annonce changement de leader** : Notification automatique quand un joueur devient #1 all-time

#### Technique
- Documentation complète avec MkDocs Material
- Support multi-backend graphique (SDL2/SFML)
- Architecture plugin pour les backends
- **Système ECS intégré** : Entity Component System pour la gestion des entités serveur
- Tests unitaires pour le formatage du score et les couleurs du rang

### Changed
- Refactoring de la couche graphique avec abstraction
- Calcul du rang utilise maintenant l'agrégation MongoDB pour joueurs uniques
- Optimisation du système de collision avec détection AABB par paires
- Amélioration de la gestion des groupes d'entités avec cache optimisé

### Fixed
- Correction des fuites mémoire dans le système ECS
- Fix du calcul de rang qui comptait les scores au lieu des joueurs uniques
- Correction du redimensionnement et des clics souris avec letterboxing
- Correction des touches coincées après utilisation du chat
- Correction du clignotement et de l'overlay du LeaderboardScene
- Fix crash SIGABRT dans TCPAdminServer lors de déconnexion client
- Remplacement de strncpy par snprintf pour éviter les buffer overflows

### Security
- Authentification par token 256-bit pour TCPAdminServer
- TCPAdminServer bind sur localhost uniquement (127.0.0.1)
- Filtrage des commandes dangereuses (quit, exit, zoom, interact, net)

---

## [1.0.0] - 2024-XX-XX

### Added
- Jeu R-Type fonctionnel client/serveur
- Système ECS complet
- Communication UDP avec Boost.ASIO
- Backend SDL2
- Backend SFML
- Configuration JSON
- Logging avec spdlog
- Support Linux et Windows

### Architecture
- Architecture hexagonale (Ports & Adapters)
- Système de plugins pour backends graphiques
- Sérialisation réseau custom

---

## Types de Changements

- **Added** : Nouvelles fonctionnalités
- **Changed** : Modifications de fonctionnalités existantes
- **Deprecated** : Fonctionnalités qui seront supprimées
- **Removed** : Fonctionnalités supprimées
- **Fixed** : Corrections de bugs
- **Security** : Corrections de vulnérabilités

---

## Convention de Versionnement

```
MAJOR.MINOR.PATCH

MAJOR : Changements incompatibles (breaking changes)
MINOR : Nouvelles fonctionnalités rétrocompatibles
PATCH : Corrections de bugs rétrocompatibles
```

### Exemples

| Version | Type | Description |
|---------|------|-------------|
| 2.0.0 | MAJOR | Nouveau protocole réseau incompatible |
| 1.1.0 | MINOR | Ajout d'un nouveau type d'ennemi |
| 1.0.1 | PATCH | Fix crash au démarrage |
