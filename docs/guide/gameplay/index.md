---
tags:
  - guide
  - gameplay
---

# Gameplay

Bienvenue dans R-Type ! Découvrez les mécaniques de jeu et maîtrisez votre vaisseau.

## Le Concept

R-Type est un **shoot 'em up horizontal** (shmup) multijoueur coopératif jusqu'à 4 joueurs.

```mermaid
flowchart LR
    A[Esquiver] --> B[Tirer]
    B --> C[Survivre]
    C --> D[Gagner !]

    style D fill:#f59e0b,color:#000
```

---

## Objectifs

| Objectif | Description |
|----------|-------------|
| **Survivre** | Évitez ennemis et projectiles |
| **Éliminer** | Détruisez toutes les vagues |
| **Coopérer** | Jouez en équipe |
| **Scorer** | Maximisez votre score |

---

## Types d'Ennemis

5 types avec comportements uniques :

| Ennemi | Comportement | Danger |
|--------|--------------|--------|
| **Basique** | Ligne droite | ⭐ |
| **Zigzag** | Mouvement erratique | ⭐⭐ |
| **Suiveur** | Vous traque | ⭐⭐⭐ |
| **Tireur** | Projectiles | ⭐⭐⭐⭐ |
| **Boss** | Fin de niveau | ⭐⭐⭐⭐⭐ |

---

## Système de Vagues

```mermaid
flowchart TB
    W1[Vague 1<br/>Basiques] --> W2[Vague 2<br/>+ Zigzags]
    W2 --> W3[Vague 3<br/>+ Suiveurs]
    W3 --> W4[Vague 4<br/>+ Tireurs]
    W4 --> BOSS[BOSS]

    style BOSS fill:#dc2626,color:#fff
```

---

## Guides

<div class="grid-cards">
  <div class="card">
    <h3><a href="controles/">🎮 Contrôles</a></h3>
    <p>Clavier et manette</p>
  </div>
  <div class="card">
    <h3><a href="multijoueur/">👥 Multijoueur</a></h3>
    <p>Créer/rejoindre une partie</p>
  </div>
  <div class="card">
    <h3><a href="voice-chat/">🎤 Voice Chat</a></h3>
    <p>Communication vocale</p>
  </div>
</div>
