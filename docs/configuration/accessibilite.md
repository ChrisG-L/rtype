---
tags:
  - configuration
  - accessibilite
---

# Accessibilité

## Modes Daltonien (5 modes)

| Mode | Type | Description |
|------|------|-------------|
| `normal` | Standard | Couleurs d'origine |
| `protanopia` | Rouge | Sans cônes rouges |
| `deuteranopia` | Vert | Sans cônes verts |
| `tritanopia` | Bleu | Sans cônes bleus |
| `achromatopsia` | Mono | Nuances de gris |

### Configuration

```json
{
  "accessibility": {
    "color_mode": "deuteranopia"
  }
}
```

### Implémentation

Filtres appliqués via **shaders post-processing** :

```glsl
vec3 applyColorBlindFilter(vec3 color, int mode) {
    mat3 filter;
    if (mode == PROTANOPIA) {
        filter = mat3(
            0.567, 0.433, 0.0,
            0.558, 0.442, 0.0,
            0.0,   0.242, 0.758
        );
    }
    return filter * color;
}
```

---

## Rebinding (7 touches)

Actions personnalisables :

1. Haut
2. Bas
3. Gauche
4. Droite
5. Tir
6. Pause
7. Voice Chat

### Configuration

**In-Game** : Options → Contrôles

**Fichier** `config/controls.json` :
```json
{
  "keyboard": {
    "move_up": "W",
    "move_down": "S",
    "move_left": "A",
    "move_right": "D",
    "shoot": "Space",
    "pause": "Escape",
    "voice_chat": "V"
  }
}
```

---

## Taille de Police

| Taille | Échelle |
|--------|---------|
| `small` | 80% |
| `medium` | 100% |
| `large` | 125% |
| `xlarge` | 150% |
