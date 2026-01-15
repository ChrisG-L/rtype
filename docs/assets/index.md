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
├── spaceship/         # Sprites (vaisseaux, missiles, backgrounds)
├── audio/             # Sons et musique
├── fonts/             # Polices de caractères
├── shaders/           # Shaders GLSL (daltonisme)
└── login/             # Images écran de connexion
```

---

## Formats Supportés

| Type | Formats | Recommandé |
|------|---------|------------|
| Sprites | PNG, JPG | PNG (transparence) |
| Musique | OGG, MP3 | OGG (compression) |
| SFX | MP3, WAV, OGG | MP3 ou WAV |
| Fonts | TTF, OTF | TTF |
| Shaders | GLSL (.vert, .frag) | GLSL 330 |

---

## Chargement

### Sprites

```cpp
// Chargement via IWindow
_context.window->loadTexture("ship", "assets/spaceship/Ship1.png");
_context.window->loadTexture("missile", "assets/spaceship/missile.png");

// Chargement de plusieurs skins
for (int i = 1; i <= 6; i++) {
    std::string key = "ship" + std::to_string(i);
    std::string path = "assets/spaceship/Ship" + std::to_string(i) + ".png";
    _context.window->loadTexture(key, path);
}
```

### Sons

```cpp
auto& audio = audio::AudioManager::getInstance();

// Effets sonores
audio.loadSound("shoot", "assets/audio/shoot.mp3");
audio.loadSound("hit", "assets/audio/hit.mp3");

// Musique de fond (si disponible)
audio.loadMusic("bgm", "assets/audio/background.ogg");
audio.playMusic("bgm", true);  // loop
```

### Fonts

```cpp
// Chargement d'une police
_context.window->loadFont("main", "assets/fonts/ARIA.TTF");

// Rendu de texte
_context.window->drawText("main", "Score: 1000", 10, 10, 24, {255, 255, 255, 255});
```

### Shaders

```cpp
// Chargement shader daltonisme (SFML uniquement)
if (_context.window->supportsShaders()) {
    _context.window->loadShader("colorblind",
        "assets/shaders/colorblind.vert",
        "assets/shaders/colorblind.frag");
}
```

---

## Documentation

<div class="grid-cards">
  <div class="card">
    <h3><a href="sprites/">Sprites</a></h3>
    <p>Vaisseaux, missiles, backgrounds</p>
  </div>
  <div class="card">
    <h3><a href="sounds/">Sons</a></h3>
    <p>Effets sonores et musique</p>
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
| Ships | `ShipN.png` | `Ship1.png`, `Ship6.png` |
| Projectiles | `type.png` | `missile.png` |
| Backgrounds | `scene.jpg` | `lobby_room.jpg` |
| SFX | `action.mp3` | `shoot.mp3`, `hit.mp3` |
| Music | `context.ogg` | `background.ogg` |

---

## Fichiers Actuels

### spaceship/

| Fichier | Dimensions | Description |
|---------|------------|-------------|
| `Ship1.png` - `Ship6.png` | 64x64 | 6 skins de vaisseaux |
| `missile.png` | 32x32 | Projectile |
| `lobby_room.jpg` | - | Background lobby |
| `bedroom.jpg` | - | Background room |
| `first_door.jpg` | - | Décor porte 1 |
| `second_door.jpg` | - | Décor porte 2 |

### audio/

| Fichier | Format | Description |
|---------|--------|-------------|
| `shoot.mp3` | MP3 | Son de tir |
| `hit.mp3` | MP3 | Son d'impact |

### fonts/

| Fichier | Description |
|---------|-------------|
| `ARIA.TTF` | Police principale |

### shaders/

| Fichier | Description |
|---------|-------------|
| `colorblind.vert` | Vertex shader passthrough |
| `colorblind.frag` | Fragment shader daltonisme |

### login/

| Fichier | Description |
|---------|-------------|
| `loginMenuBg.jpg` | Background menu login |
| `loginInput.jpg` | Image champ de saisie |
