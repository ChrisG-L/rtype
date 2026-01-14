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
sprites/
├── player/
│   ├── ship1.png          # Vaisseau bleu
│   ├── ship2.png          # Vaisseau rouge
│   ├── ship3.png          # Vaisseau vert
│   └── ship4.png          # Vaisseau jaune
│
├── enemies/
│   ├── basic.png          # Ennemi simple
│   ├── zigzag.png         # Ennemi zigzag
│   ├── follower.png       # Ennemi suiveur
│   ├── shooter.png        # Ennemi tireur
│   └── boss/
│       ├── boss_idle.png
│       └── boss_attack.png
│
├── projectiles/
│   ├── player_missile.png
│   └── enemy_missile.png
│
├── effects/
│   ├── explosion_anim.png  # Spritesheet 8 frames
│   ├── hit_anim.png        # Spritesheet 4 frames
│   └── powerup_glow.png
│
└── ui/
    ├── health_bar.png
    ├── button_normal.png
    ├── button_hover.png
    └── icons/
```

---

## Joueurs

### Vaisseaux

| Sprite | Dimensions | Frames | Description |
|--------|------------|--------|-------------|
| `ship1.png` | 64x32 | 1 | Vaisseau bleu (P1) |
| `ship2.png` | 64x32 | 1 | Vaisseau rouge (P2) |
| `ship3.png` | 64x32 | 1 | Vaisseau vert (P3) |
| `ship4.png` | 64x32 | 1 | Vaisseau jaune (P4) |

### Preview

```
┌────────────────────────────────────┐
│                                    │
│   ▲                               │
│  ███▶    Ship 1 (64x32)           │
│   ▼                               │
│                                    │
└────────────────────────────────────┘
```

---

## Ennemis

### Types

| Sprite | Dimensions | Frames | Hitbox |
|--------|------------|--------|--------|
| `basic.png` | 48x32 | 1 | 48x32 |
| `zigzag.png` | 48x32 | 1 | 48x32 |
| `follower.png` | 40x40 | 1 | 40x40 |
| `shooter.png` | 56x40 | 1 | 56x40 |
| `boss_idle.png` | 256x128 | 1 | 256x128 |
| `boss_attack.png` | 256x128 | 4 | 256x128 |

### Boss

```
┌──────────────────────────────────────────────────────────────┐
│                                                              │
│    ████████████████████████████████████████████████████      │
│    ██████████████████  BOSS  ████████████████████████        │
│    ████████████████████████████████████████████████████      │
│                                                              │
│                    256 x 128 pixels                          │
│                                                              │
└──────────────────────────────────────────────────────────────┘
```

---

## Projectiles

| Sprite | Dimensions | Vitesse | Dégâts |
|--------|------------|---------|--------|
| `player_missile.png` | 16x8 | 600 px/s | 10 |
| `enemy_missile.png` | 12x6 | 300 px/s | 20 |

---

## Effets

### Animations (Spritesheets)

| Sprite | Dimensions | Frames | FPS |
|--------|------------|--------|-----|
| `explosion_anim.png` | 512x64 (8x64) | 8 | 24 |
| `hit_anim.png` | 128x32 (4x32) | 4 | 30 |

### Spritesheet Layout

```
┌────┬────┬────┬────┬────┬────┬────┬────┐
│ 1  │ 2  │ 3  │ 4  │ 5  │ 6  │ 7  │ 8  │
│    │    │    │    │    │    │    │    │
└────┴────┴────┴────┴────┴────┴────┴────┘
  explosion_anim.png (8 frames x 64px)
```

---

## Interface

### Health Bar

```
┌──────────────────────────────────────┐
│ ████████████████████░░░░░░░░░░░░░░░░ │  80%
└──────────────────────────────────────┘
   Dimensions: 200x20
```

### Boutons

| État | Sprite | Dimensions |
|------|--------|------------|
| Normal | `button_normal.png` | 200x50 |
| Hover | `button_hover.png` | 200x50 |
| Pressed | `button_pressed.png` | 200x50 |

---

## Utilisation dans le Code

### Chargement

```cpp
class TextureManager {
    std::unordered_map<std::string, Texture> textures_;

public:
    void loadAll() {
        // Players
        textures_["ship1"] = load("assets/sprites/player/ship1.png");
        textures_["ship2"] = load("assets/sprites/player/ship2.png");

        // Enemies
        textures_["basic"] = load("assets/sprites/enemies/basic.png");
        textures_["boss"] = load("assets/sprites/enemies/boss/boss_idle.png");

        // Effects
        textures_["explosion"] = load("assets/sprites/effects/explosion_anim.png");
    }

    const Texture& get(const std::string& name) const {
        return textures_.at(name);
    }
};
```

### Animation

```cpp
class Animation {
    Texture spritesheet_;
    int frameWidth_;
    int frameCount_;
    float frameDuration_;
    float elapsed_ = 0;
    int currentFrame_ = 0;

public:
    Animation(const Texture& sheet, int frameW, int count, float fps)
        : spritesheet_(sheet)
        , frameWidth_(frameW)
        , frameCount_(count)
        , frameDuration_(1.0f / fps)
    {}

    void update(float dt) {
        elapsed_ += dt;
        if (elapsed_ >= frameDuration_) {
            elapsed_ = 0;
            currentFrame_ = (currentFrame_ + 1) % frameCount_;
        }
    }

    void render(Renderer& r, float x, float y) {
        r.drawSprite(spritesheet_,
            {currentFrame_ * frameWidth_, 0, frameWidth_, spritesheet_.height()},
            {x, y}
        );
    }
};
```

---

## Guidelines de Création

### Style Visuel

- **Palette**: 16 couleurs max par sprite
- **Style**: Pixel art rétro
- **Outline**: Contour noir 1px
- **Transparence**: Canal alpha PNG

### Dimensions Recommandées

| Entité | Taille | Raison |
|--------|--------|--------|
| Player | 64x32 | Lisibilité |
| Enemy (small) | 48x32 | Proportionnel |
| Enemy (large) | 256x128 | Impact visuel |
| Projectile | 16x8 | Vitesse perçue |
| UI Button | 200x50 | Zone de clic |
