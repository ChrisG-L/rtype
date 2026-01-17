---
tags:
  - technique
  - gameplay
  - combat
  - equilibrage
---

# Systeme de Combat

Documentation complete des degats, HP, hitboxes et statistiques.

## Vue d'Ensemble

### Philosophie d'Equilibrage

1. **DPS normalise** : Toutes les armes ~65-70 DPS au niveau 0
2. **Choix tactique** : Chaque arme a un cas d'usage optimal
3. **Progression naturelle** : Ennemis plus difficiles avec les vagues
4. **Boss epique** : Combat de ~22 secondes avec 3 phases

### Constantes Globales

| Constante | Valeur |
|-----------|--------|
| Resolution | 1920x1080 px |
| Tick rate serveur | 60 Hz |
| Broadcast rate | 20 Hz |

---

## Joueur

### Statistiques

| Stat | Valeur |
|------|--------|
| HP Maximum | 100 |
| Vitesse base | 200 px/s |
| Vitesse max | 380 px/s |
| Hitbox | 64x30 px |

### Niveaux de Vitesse

| Niveau | Multiplicateur | Vitesse |
|--------|----------------|---------|
| 0 | 1.0x | 200 px/s |
| 1 | 1.3x | 260 px/s |
| 2 | 1.6x | 320 px/s |
| 3 | 1.9x | 380 px/s |

### Sources de Degats

| Source | Degats | Survie |
|--------|--------|--------|
| Missile ennemi | 20 HP | 5 hits |
| Contact ennemi | 20 HP | 5 hits |
| Attaque Boss | 20 HP | 5 hits |

---

## Armes du Joueur

4 types d'armes selectionnables avec Q/E.

### Comparatif Niveau 0

| Arme | Degats | Vitesse | Cooldown | DPS | Special |
|------|--------|---------|----------|-----|---------|
| Standard | 20 | 600 px/s | 0.30s | 66.7 | Fiable |
| Spread | 8x3 | 550 px/s | 0.40s | 60.0 | Eventail |
| Laser | 12 | 900 px/s | 0.18s | 66.7 | Rapide |
| Homing | 50 | 350 px/s | 0.70s | 71.4 | Auto-guide |

### Hitbox Projectiles

| Type | Taille |
|------|--------|
| Standard/Laser/Homing | 16x8 px |
| Spread | 16x8 px (x3) |

---

## Niveaux d'Armes

Ramasser un **WeaponCrystal** augmente le niveau de toutes les armes.

### Bonus par Niveau

| Niveau | Degats | Cooldown | Vitesse |
|--------|--------|----------|---------|
| Lv.0 | x1.00 | x1.00 | x1.00 |
| Lv.1 | x1.15 | x0.95 | x1.00 |
| Lv.2 | x1.30 | x0.90 | x1.00 |
| Lv.3 | x1.50 | x0.85 | x1.10 |

### DPS par Arme et Niveau

| Arme | Lv.0 | Lv.3 | Gain |
|------|------|------|------|
| Standard | 66.7 | 117.6 | +76% |
| Spread | 60.0 | 105.9 | +76% |
| Laser | 66.7 | 117.6 | +76% |
| Homing | 71.4 | 126.1 | +77% |

### Temps pour tuer Boss (1500 HP)

| Arme | Lv.0 | Lv.3 |
|------|------|------|
| Standard | 22.5s | 12.8s |
| Spread | 25.0s | 14.2s |
| Laser | 22.5s | 12.8s |
| Homing | 21.0s | 11.9s |

---

## Wave Cannon

| Niveau | Temps | Degats | Largeur |
|--------|-------|--------|---------|
| Lv.1 | 0.6s | 50 | 20px |
| Lv.2 | 1.3s | 100 | 35px |
| Lv.3 | 2.2s | 250 | 55px |

**Note** : Le Wave Cannon au Lv.3 inflige 250 degats, soit ~17% du boss en un tir.

---

## Force Pod & Bits

### Force Pod

| Propriete | Valeur |
|-----------|--------|
| Taille | 32x32 px |
| Degats contact | 30 |
| Invincible | Oui |

### Bit Devices

| Propriete | Valeur |
|-----------|--------|
| Taille | 24x24 px |
| Degats contact | 15 |
| Rayon orbite | 50 px |
| Invincible | Oui |

---

## Ennemis

### Statistiques par Type

| Type | HP | Vitesse | Tir | Points |
|------|-----|---------|-----|--------|
| Basic | 40 | -120 | 2.5s | 100 |
| Tracker | 35 | -100 | 2.0s | 150 |
| Zigzag | 30 | -140 | 3.0s | 120 |
| Fast | 25 | -220 | 1.5s | 180 |
| Bomber | 80 | -80 | 1.0s | 250 |
| POWArmor | 60 | -90 | 4.0s | 200 |

### Hitboxes Ennemis

| Type | Taille |
|------|--------|
| Standard | 40x40 px |
| Bomber | 50x50 px |
| POWArmor | 45x45 px |

### Tirs pour Tuer (Lv.0)

| Ennemi | Standard | Spread | Laser | Homing |
|--------|----------|--------|-------|--------|
| Basic | 2 | 5 | 4 | 1 |
| Tracker | 2 | 5 | 3 | 1 |
| Zigzag | 2 | 4 | 3 | 1 |
| Fast | 2 | 4 | 3 | 1 |
| Bomber | 4 | 10 | 7 | 2 |
| POWArmor | 3 | 8 | 5 | 2 |

---

## Boss

### Statistiques Nemesis

| Propriete | Valeur |
|-----------|--------|
| HP Base | 1500 |
| HP/Cycle | +500 |
| Hitbox | 150x120 px |
| Points | 5000 |
| Spawn | Wave 10, 20, 30... |

### Phases

| Phase | HP Seuil | Comportement |
|-------|----------|--------------|
| Phase 1 | 100-65% | Attaques basiques |
| Phase 2 | 65-30% | + Patterns complexes |
| Phase 3 | 30-0% | Enrage, attaques rapides |

### Cycle de Respawn

| Cycle | Wave | HP |
|-------|------|-----|
| 1 | 10 | 1500 |
| 2 | 20 | 2000 |
| 3 | 30 | 2500 |
| 4 | 40 | 3000 |

---

## Systeme de Score

### Points par Ennemi

| Ennemi | Points |
|--------|--------|
| Basic | 100 |
| Tracker | 150 |
| Zigzag | 120 |
| Fast | 180 |
| Bomber | 250 |
| POWArmor | 200 |
| Boss | 5000 |

### Combo Multiplier

| Propriete | Valeur |
|-----------|--------|
| Increment | +0.1x par kill |
| Maximum | 3.0x |
| Decay | 3.0s sans kill |

### Formule

```cpp
uint32_t calculateScore(uint16_t basePoints, float combo) {
    return static_cast<uint32_t>(basePoints * combo);
}
```

---

## Systeme de Vagues

### Formules de Spawn

```cpp
// Ennemis par wave
uint8_t enemiesPerWave(uint16_t wave) {
    return std::min(5 + wave / 2, 16);
}

// Intervalle entre spawns
float spawnInterval(uint16_t wave) {
    return std::max(2.0f - wave * 0.05f, 0.5f);
}
```

### Progression

| Wave | Ennemis | Intervalle | Types |
|------|---------|------------|-------|
| 1-5 | 5-7 | 2.0-1.75s | Basic, Tracker |
| 6-10 | 8-10 | 1.7-1.5s | + Zigzag, Fast |
| 11-15 | 10-12 | 1.45-1.25s | + Bomber |
| 16-20 | 13-15 | 1.2-1.0s | + POWArmor |
| 21+ | 16 | 0.5s | Tous types |

---

## Hitboxes (AABB)

### Constantes (AABB.hpp)

```cpp
namespace Hitboxes {
    // Joueur
    constexpr float SHIP_WIDTH = 50.0f;
    constexpr float SHIP_HEIGHT = 30.0f;

    // Missiles
    constexpr float MISSILE_WIDTH = 16.0f;
    constexpr float MISSILE_HEIGHT = 8.0f;

    // Ennemis
    constexpr float ENEMY_WIDTH = 40.0f;
    constexpr float ENEMY_HEIGHT = 40.0f;

    // Boss
    constexpr float BOSS_WIDTH = 150.0f;
    constexpr float BOSS_HEIGHT = 120.0f;

    // Power-ups
    constexpr float POWERUP_SIZE = 32.0f;

    // Force Pod
    constexpr float FORCE_SIZE = 32.0f;

    // Bit Device
    constexpr float BIT_SIZE = 24.0f;
}
```

---

## Affichage du Rang en Jeu

Pendant la partie, le rang global et le meilleur score personnel sont affiches dans le HUD.

### Fonctionnalites

| Element | Description |
|---------|-------------|
| Rang global | Position actuelle (#1, #2, etc.) |
| Meilleur score | Record personnel a battre |
| Mise a jour | Toutes les 10 secondes |
| Indicateur NEW! | Affiche quand le score depasse le record |

### Couleurs du Rang

| Position | Couleur |
|----------|---------|
| #1 | Or (255, 215, 0) |
| #2 | Argent (192, 192, 192) |
| #3 | Bronze (205, 127, 50) |
| Top 10 | Bleu clair (100, 200, 255) |
| Top 50 | Vert (100, 255, 150) |
| Autres | Gris (200, 200, 200) |

### Format du Score

| Seuil | Format | Exemple |
|-------|--------|---------|
| < 1000 | Brut | BEST: 850 |
| >= 1000 | K | BEST: 32.6K |
| >= 1000000 | M | BEST: 1.2M |

### Comportement Live

- Le meilleur score suit le score actuel quand il est depasse
- Le texte devient vert avec "NEW!" quand un nouveau record est etabli
- Exemple : `BEST: 17.3K NEW!`

---

## Leaderboard - Tri par Colonnes

Le leaderboard permet de trier les entrees par differentes colonnes.

### Colonnes Triables

| Colonne | Description | Direction par defaut |
|---------|-------------|---------------------|
| SCORE | Score total | Descendant (plus haut d'abord) |
| WAVE | Vague atteinte | Descendant |
| KILLS | Nombre de kills | Descendant |
| TIME | Duree de la partie | Descendant |

### Comportement

- Cliquer sur une colonne trie par cette colonne
- Cliquer a nouveau inverse l'ordre (ascendant <-> descendant)
- La colonne active est affichee en or avec un indicateur (`^` ou `v`)
- Changer de periode (ALL TIME/WEEKLY/MONTHLY) reset le tri a RANK ascendant

### Indicateurs Visuels

| Indicateur | Signification |
|------------|---------------|
| `^` | Tri ascendant (plus petit d'abord) |
| `v` | Tri descendant (plus grand d'abord) |
| Couleur or | Colonne de tri active |
| Couleur grise | Colonne inactive |

---

## References Code

| Fichier | Description |
|---------|-------------|
| `Protocol.hpp` | Constantes et structures |
| `GameWorld.cpp` | Logique serveur |
| `AABB.hpp` | Systeme de collision |
| `GameScene.cpp` | Rendu et HUD |
| `MongoDBLeaderboardRepository.cpp` | Calcul du rang unique par joueur |
| `LeaderboardScene.cpp` | Affichage et tri du leaderboard |
