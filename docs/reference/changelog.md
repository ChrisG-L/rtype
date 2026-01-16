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
- Documentation complète avec MkDocs Material
- Support multi-backend graphique (SDL2/SFML)
- Architecture plugin pour les backends
- Affichage du rang global en temps réel pendant la partie
- Affichage du meilleur score personnel avec indicateur "NEW!"
- Mise à jour périodique du rang (toutes les 10 secondes)
- Tests unitaires pour le formatage du score et les couleurs du rang

### Changed
- Refactoring de la couche graphique avec abstraction
- Calcul du rang utilise maintenant l'agrégation MongoDB pour joueurs uniques

### Fixed
- Correction des fuites mémoire dans le système ECS
- Fix du calcul de rang qui comptait les scores au lieu des joueurs uniques

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
