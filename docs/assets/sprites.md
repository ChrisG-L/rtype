---
tags:
  - assets
  - sprites
  - graphiques
---

# Sprites

Ressources graphiques du jeu R-Type.

## Structure

```
assets/
├── spaceship/
│   ├── Ship1.png          # Vaisseau joueur 1
│   ├── Ship2.png          # Vaisseau joueur 2
│   ├── Ship3.png          # Vaisseau joueur 3
│   ├── Ship4.png          # Vaisseau joueur 4
│   ├── Ship5.png          # Skin alternatif 1
│   ├── Ship6.png          # Skin alternatif 2
│   ├── missile.png        # Projectile
│   ├── lobby_room.jpg     # Background lobby
│   ├── bedroom.jpg        # Background room
│   ├── first_door.jpg     # Décor porte
│   └── second_door.jpg    # Décor porte
│
├── login/
│   ├── loginMenuBg.jpg    # Background écran login
│   └── loginInput.jpg     # Champ de saisie
│
└── shaders/
    ├── colorblind.vert    # Shader daltonisme (vertex)
    └── colorblind.frag    # Shader daltonisme (fragment)
```

---

## Vaisseaux (Ships)

### Fichiers Disponibles

| Sprite | Dimensions | Hitbox | Description |
|--------|------------|--------|-------------|
| `Ship1.png` | 64x64 | 64x30 | Skin par défaut |
| `Ship2.png` | 64x64 | 64x30 | Skin 2 |
| `Ship3.png` | 64x64 | 64x30 | Skin 3 |
| `Ship4.png` | 64x64 | 64x30 | Skin 4 |
| `Ship5.png` | 64x64 | 64x30 | Skin 5 |
| `Ship6.png` | 64x64 | 64x30 | Skin 6 |

!!! note "Hitbox vs Sprite"
    Le sprite fait 64x64 pixels mais la hitbox de collision est 64x30.
    Défini dans `src/common/collision/AABB.hpp`.

### Chargement dans le Code

```cpp
// GameScene.cpp - Chargement des skins
for (int i = 1; i <= 6; i++) {
    std::string key = "ship" + std::to_string(i);
    std::string path = "assets/spaceship/Ship" + std::to_string(i) + ".png";
    _context.window->loadTexture(key, path);
}

// Texture par défaut
_context.window->loadTexture(SHIP_TEXTURE_KEY, "assets/spaceship/Ship1.png");
```

---

## Projectiles

| Sprite | Dimensions | Hitbox | Vitesse |
|--------|------------|--------|---------|
| `missile.png` | 32x32 | 16x8 | 600 px/s |

!!! note "Hitbox vs Sprite"
    Le sprite fait 32x32 pixels mais la hitbox de collision est 16x8.

### Chargement

```cpp
_context.window->loadTexture(MISSILE_TEXTURE_KEY, "assets/spaceship/missile.png");
```

---

## Ennemis

Les ennemis utilisent actuellement les mêmes sprites que les joueurs.

| Usage | Sprite | Hitbox |
|-------|--------|--------|
| Ennemi | `Ship1.png` | 40x40 |

```cpp
_context.window->loadTexture(ENEMY_TEXTURE_KEY, "assets/spaceship/Ship1.png");
```

---

## Backgrounds

| Sprite | Format | Usage |
|--------|--------|-------|
| `lobby_room.jpg` | JPG | Background écran lobby |
| `bedroom.jpg` | JPG | Background room/jeu |
| `first_door.jpg` | JPG | Décor porte niveau 1 |
| `second_door.jpg` | JPG | Décor porte niveau 2 |
| `loginMenuBg.jpg` | JPG | Background écran login |
| `loginInput.jpg` | JPG | Image champ de saisie |

---

## Hitboxes (Collisions)

Définies dans `src/common/collision/AABB.hpp` :

```cpp
namespace collision::Hitboxes {
    static constexpr float SHIP_WIDTH = 64.0f;
    static constexpr float SHIP_HEIGHT = 30.0f;
    static constexpr float MISSILE_WIDTH = 16.0f;
    static constexpr float MISSILE_HEIGHT = 8.0f;
    static constexpr float ENEMY_WIDTH = 40.0f;
    static constexpr float ENEMY_HEIGHT = 40.0f;
}
```

### Visualisation

```
Ship (64x30 hitbox sur sprite 64x64):
┌────────────────────────────────────────────────────────────────┐
│                        (sprite 64x64)                          │
│   ┌──────────────────────────────────────────────────────┐     │
│   │              HITBOX (64x30)                          │     │
│   └──────────────────────────────────────────────────────┘     │
│                                                                │
└────────────────────────────────────────────────────────────────┘

Missile (16x8 hitbox sur sprite 32x32):
┌────────────────────────────────┐
│       (sprite 32x32)           │
│       ┌──────────┐             │
│       │HITBOX 16x8             │
│       └──────────┘             │
└────────────────────────────────┘
```

---

## Utilisation dans le Code

### GameScene - Chargement Complet

```cpp
void GameScene::loadResources() {
    // Charger tous les skins de vaisseaux
    for (int i = 1; i <= 6; i++) {
        std::string key = "ship" + std::to_string(i);
        std::string path = "assets/spaceship/Ship" + std::to_string(i) + ".png";
        _context.window->loadTexture(key, path);
    }

    // Texture par défaut du joueur
    _context.window->loadTexture(SHIP_TEXTURE_KEY, "assets/spaceship/Ship1.png");

    // Missile
    _context.window->loadTexture(MISSILE_TEXTURE_KEY, "assets/spaceship/missile.png");

    // Ennemi (utilise Ship1 pour l'instant)
    _context.window->loadTexture(ENEMY_TEXTURE_KEY, "assets/spaceship/Ship1.png");

    // Police
    _context.window->loadFont(FONT_KEY, "assets/fonts/ARIA.TTF");
}
```

### Rendu

```cpp
void GameScene::render() {
    auto& window = *_context.window;

    // Dessiner les joueurs
    for (const auto& player : _players) {
        window.drawSprite(SHIP_TEXTURE_KEY, player.x, player.y, 64, 64);
    }

    // Dessiner les missiles
    for (const auto& missile : _missiles) {
        window.drawSprite(MISSILE_TEXTURE_KEY, missile.x, missile.y, 32, 16);
    }
}
```

---

## Constantes (GameScene.hpp)

```cpp
// Dimensions d'affichage
static constexpr float SHIP_WIDTH = 64.0f;
static constexpr float SHIP_HEIGHT = 64.0f;
static constexpr float MISSILE_WIDTH = 32.0f;
static constexpr float MISSILE_HEIGHT = 16.0f;

// Vitesse
static constexpr float MOVE_SPEED = 200.0f;
static constexpr float MISSILE_SPEED = 600.0f;
```

---

## Ajout de Nouveaux Sprites

Pour ajouter de nouveaux assets :

1. Placer le fichier PNG/JPG dans `assets/spaceship/`
2. Charger avec `loadTexture()` dans la scène appropriée
3. Si nouvelle entité, définir la hitbox dans `AABB.hpp`

### Format Recommandé

- **Format** : PNG avec transparence (canal alpha)
- **Dimensions** : Puissance de 2 (32x32, 64x64, 128x128)
- **Style** : Pixel art ou sprites détaillés
