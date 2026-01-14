---
tags:
  - guide
  - débutant
---

# Guide Utilisateur

Bienvenue ! Cette section vous accompagne de l'installation à votre première partie.

<div class="grid-cards">
  <div class="card">
    <div class="card-icon">📥</div>
    <h3><a href="installation/">Installation</a></h3>
    <p>Prérequis et dépendances</p>
  </div>

  <div class="card">
    <div class="card-icon">🚀</div>
    <h3><a href="quickstart/">Démarrage Rapide</a></h3>
    <p>Première partie en 5 minutes</p>
  </div>

  <div class="card">
    <div class="card-icon">🔨</div>
    <h3><a href="compilation/">Compilation</a></h3>
    <p>Options de build avancées</p>
  </div>

  <div class="card card-highlight">
    <div class="card-icon">🎮</div>
    <h3><a href="gameplay/">Gameplay</a></h3>
    <p>Contrôles, multijoueur, voice chat</p>
  </div>
</div>

---

## Prérequis

| Composant | Version |
|-----------|---------|
| **OS** | Linux (Ubuntu 22.04+), Windows 10+ |
| **Compilateur** | GCC 11+ ou Clang 15+ |
| **CMake** | 3.20+ |
| **vcpkg** | Latest |

---

## Flux

```mermaid
flowchart LR
    A[Installation] --> B[Compilation]
    B --> C[Serveur]
    C --> D[Client]
    D --> E[Jouer !]

    style A fill:#7c3aed,color:#fff
    style E fill:#f59e0b,color:#000
```

!!! tip "Premier lancement ?"
    Suivez le [Démarrage Rapide](quickstart.md) pour une expérience guidée.
