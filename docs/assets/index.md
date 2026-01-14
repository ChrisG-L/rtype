---
tags:
  - assets
  - ressources
---

# Assets

Ressources graphiques et audio du projet R-Type.

## Vue d'Ensemble

```
assets/
├── sprites/           # Images et animations
│   ├── player/       # Vaisseaux joueurs
│   ├── enemies/      # Ennemis
│   ├── projectiles/  # Missiles et lasers
│   ├── effects/      # Explosions, particules
│   └── ui/           # Interface utilisateur
│
├── sounds/           # Effets sonores
│   ├── sfx/          # Sons de jeu
│   └── music/        # Musiques
│
└── fonts/            # Polices de caractères
```

---

## Formats Supportés

| Type | Formats | Recommandé |
|------|---------|------------|
| Sprites | PNG, BMP | PNG (transparence) |
| Musique | OGG, WAV | OGG (compression) |
| SFX | WAV, OGG | WAV (faible latence) |
| Fonts | TTF, OTF | TTF |

---

## Chargement

### Sprites

```cpp
// Chargement d'un sprite
auto texture = renderer.loadTexture("assets/sprites/player/ship1.png");

// Spritesheet avec animation
auto spritesheet = renderer.loadSpritesheet(
    "assets/sprites/player/ship1_anim.png",
    64, 32,  // frame size
    8        // frame count
);
```

### Sons

```cpp
// Effet sonore
audio.loadSound("shoot", "assets/sounds/sfx/shoot.wav");
audio.playSound("shoot");

// Musique
audio.loadMusic("theme", "assets/sounds/music/game.ogg");
audio.playMusic("theme", true);  // loop
```

### Fonts

```cpp
// Chargement d'une police
auto font = renderer.loadFont(
    "assets/fonts/pixel.ttf",
    16  // taille
);

// Rendu de texte
renderer.drawText(font, "Score: 1000", {10, 10});
```

---

## Documentation

<div class="grid-cards">
  <div class="card">
    <h3><a href="sprites/">Sprites</a></h3>
    <p>Images et animations</p>
  </div>
  <div class="card">
    <h3><a href="sounds/">Sons</a></h3>
    <p>Musiques et effets</p>
  </div>
  <div class="card">
    <h3><a href="fonts/">Polices</a></h3>
    <p>Typographies</p>
  </div>
</div>

---

## Conventions de Nommage

| Type | Convention | Exemple |
|------|------------|---------|
| Sprites | `entity_action.png` | `player_idle.png` |
| Animations | `entity_anim.png` | `explosion_anim.png` |
| SFX | `action.wav` | `shoot.wav` |
| Music | `context_theme.ogg` | `game_theme.ogg` |

---

## Optimisations

### Sprites

- Utiliser des spritesheets pour les animations
- Taille power of 2 pour les textures (64x64, 128x128)
- Format PNG avec compression

### Audio

- SFX en WAV mono 44.1kHz pour faible latence
- Musique en OGG stereo pour compression
- Normaliser les volumes
