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

| Ennemi | Comportement | PV | Danger |
|--------|--------------|-----|--------|
| **Basic** | Mouvement sinusoïdal | 30 | ⭐ |
| **Tracker** | Suit le joueur en Y | 25 | ⭐⭐ |
| **Zigzag** | Zigzag vertical rapide | 20 | ⭐⭐ |
| **Fast** | Très rapide, petite oscillation | 15 | ⭐⭐⭐ |
| **Bomber** | Lent mais tire 2 missiles | 50 | ⭐⭐⭐⭐ |

---

## Système de Vagues

Les ennemis apparaissent progressivement selon le numéro de vague :

```mermaid
flowchart TB
    W1[Vague 1<br/>Basic] --> W2[Vague 2<br/>+ Tracker]
    W2 --> W3[Vague 3<br/>+ Zigzag]
    W3 --> W4[Vague 4<br/>+ Fast]
    W4 --> W5[Vague 5+<br/>+ Bomber]

    style W5 fill:#dc2626,color:#fff
```

!!! info "Difficulté progressive"
    Le nombre d'ennemis par vague augmente au fil du temps. Les types les plus dangereux n'apparaissent qu'après plusieurs vagues.

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
