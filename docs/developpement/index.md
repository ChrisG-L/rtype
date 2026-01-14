---
tags:
  - developpement
  - guide
---

# Guide Développement

Guide pour les contributeurs au projet R-Type.

## Vue d'Ensemble

Ce guide couvre tout ce dont vous avez besoin pour contribuer au projet R-Type.

```mermaid
flowchart LR
    Setup[Setup] --> Code[Coder]
    Code --> Test[Tester]
    Test --> PR[Pull Request]
    PR --> Review[Review]
    Review --> Merge[Merge]

    style Setup fill:#7c3aed,color:#fff
    style Merge fill:#059669,color:#fff
```

---

## Prérequis

### Outils Requis

| Outil | Version | Description |
|-------|---------|-------------|
| C++ Compiler | GCC 13+ / Clang 16+ | Support C++23 |
| CMake | 3.25+ | Build system |
| Git | 2.40+ | Version control |
| Conan | 2.0+ | Package manager |
| Python | 3.10+ | Scripts et docs |

### Dépendances

```bash
# Ubuntu/Debian
sudo apt install build-essential cmake git python3 python3-pip

# Installation Conan
pip install conan

# Profil Conan
conan profile detect
```

---

## Structure du Projet

```
rtype/
├── client/                 # Application client
│   ├── src/
│   │   ├── engine/        # Moteur de jeu
│   │   ├── scenes/        # Scènes (Menu, Game, etc.)
│   │   ├── network/       # Client réseau
│   │   └── audio/         # Audio et voice chat
│   └── CMakeLists.txt
│
├── server/                 # Application serveur
│   ├── src/
│   │   ├── application/   # Point d'entrée
│   │   ├── rooms/         # Gestion des salons
│   │   ├── game/          # Logique de jeu
│   │   └── network/       # Serveurs TCP/UDP
│   └── CMakeLists.txt
│
├── common/                 # Code partagé
│   ├── include/
│   │   └── protocol/      # Protocole réseau
│   └── src/
│
├── docs/                   # Documentation
├── tests/                  # Tests unitaires
├── assets/                 # Ressources graphiques/audio
└── CMakeLists.txt         # CMake racine
```

---

## Workflow Git

### Branches

| Branche | Description |
|---------|-------------|
| `main` | Production stable |
| `develop` | Intégration |
| `feature/*` | Nouvelles fonctionnalités |
| `fix/*` | Corrections de bugs |
| `docs/*` | Documentation |

### Commandes

```bash
# Créer une branche feature
git checkout develop
git pull origin develop
git checkout -b feature/ma-feature

# Après le développement
git add .
git commit -m "feat: description"
git push origin feature/ma-feature
```

---

## Documentation

<div class="grid-cards">
  <div class="card">
    <h3><a href="architecture/">Architecture</a></h3>
    <p>Décisions architecturales</p>
  </div>
  <div class="card">
    <h3><a href="conventions/">Conventions</a></h3>
    <p>Style de code</p>
  </div>
  <div class="card">
    <h3><a href="tests/">Tests</a></h3>
    <p>Guide de test</p>
  </div>
  <div class="card">
    <h3><a href="ci-cd/">CI/CD</a></h3>
    <p>Intégration continue</p>
  </div>
  <div class="card">
    <h3><a href="contribution/">Contribution</a></h3>
    <p>Comment contribuer</p>
  </div>
</div>

---

## Quick Start

```bash
# 1. Cloner le repo
git clone https://github.com/Pluenet-Killian/rtype.git
cd rtype

# 2. Installer les dépendances
conan install . --build=missing

# 3. Configurer CMake
cmake --preset conan-release

# 4. Compiler
cmake --build --preset conan-release

# 5. Exécuter les tests
ctest --preset conan-release
```
