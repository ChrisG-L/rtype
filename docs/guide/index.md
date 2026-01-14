---
tags:
  - guide
  - débutant
---

# Guide Utilisateur

Bienvenue dans le guide utilisateur R-Type ! Cette section vous accompagne de l'installation au lancement de votre première partie.

<div class="grid-cards">
  <div class="card">
    <div class="card-icon">📥</div>
    <h3><a href="installation/">Installation</a></h3>
    <p>Prérequis et installation des dépendances.</p>
  </div>

  <div class="card">
    <div class="card-icon">🚀</div>
    <h3><a href="quickstart/">Démarrage Rapide</a></h3>
    <p>Lancez votre première partie en 5 minutes.</p>
  </div>

  <div class="card">
    <div class="card-icon">⚙️</div>
    <h3><a href="configuration/">Configuration</a></h3>
    <p>Personnalisez le client et le serveur.</p>
  </div>

  <div class="card">
    <div class="card-icon">🔨</div>
    <h3><a href="building/">Compilation</a></h3>
    <p>Options de build et compilation avancée.</p>
  </div>
</div>

---

## Prérequis Rapides

| Composant | Version Minimum |
|-----------|-----------------|
| **OS** | Linux (Ubuntu 22.04+), Windows 10+ |
| **Compilateur** | GCC 11+ ou Clang 15+ |
| **CMake** | 3.20+ |
| **vcpkg** | Latest |

---

## Flux de Démarrage

```mermaid
flowchart LR
    A[Installation] --> B[Configuration]
    B --> C[Compilation]
    C --> D[Lancement Serveur]
    D --> E[Lancement Client]
    E --> F[Jouer !]

    style A fill:#7c3aed,color:#fff
    style F fill:#f59e0b,color:#000
```

!!! tip "Premier lancement ?"
    Suivez le [Démarrage Rapide](quickstart.md) pour une expérience guidée pas à pas.
