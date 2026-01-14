---
tags:
  - assets
  - fonts
  - typographie
---

# Polices de Caractères

Typographies utilisées dans R-Type.

## Structure

```
fonts/
├── pixel.ttf          # Police principale (UI)
├── pixel_bold.ttf     # Variante bold
├── title.ttf          # Titres
└── mono.ttf           # Code/debug
```

---

## Polices Utilisées

### Pixel (Principale)

| Propriété | Valeur |
|-----------|--------|
| Fichier | `pixel.ttf` |
| Style | Pixel art |
| Usage | Interface, textes |
| Tailles | 16, 24, 32px |
| Licence | OFL |

**Preview:**

```
ABCDEFGHIJKLMNOPQRSTUVWXYZ
abcdefghijklmnopqrstuvwxyz
0123456789
!@#$%^&*()_+-=[]{}|;':",.<>?/
```

### Title (Titres)

| Propriété | Valeur |
|-----------|--------|
| Fichier | `title.ttf` |
| Style | Display bold |
| Usage | Écrans titre, menus |
| Tailles | 48, 64px |
| Licence | OFL |

### Mono (Debug)

| Propriété | Valeur |
|-----------|--------|
| Fichier | `mono.ttf` |
| Style | Monospace |
| Usage | Debug, console |
| Tailles | 12, 14px |
| Licence | OFL |

---

## Utilisation

### Chargement

```cpp
class FontManager {
    std::unordered_map<std::string, Font> fonts_;

public:
    void loadAll(Renderer& renderer) {
        fonts_["pixel_16"] = renderer.loadFont(
            "assets/fonts/pixel.ttf", 16
        );
        fonts_["pixel_24"] = renderer.loadFont(
            "assets/fonts/pixel.ttf", 24
        );
        fonts_["pixel_32"] = renderer.loadFont(
            "assets/fonts/pixel.ttf", 32
        );
        fonts_["title"] = renderer.loadFont(
            "assets/fonts/title.ttf", 64
        );
        fonts_["mono"] = renderer.loadFont(
            "assets/fonts/mono.ttf", 14
        );
    }

    const Font& get(const std::string& name) const {
        return fonts_.at(name);
    }
};
```

### Rendu

```cpp
// Texte simple
renderer.drawText(fontManager.get("pixel_24"),
                  "Score: 1000",
                  {10, 10},
                  Color::White);

// Texte centré
void drawCenteredText(Renderer& r, const Font& font,
                      const std::string& text, float y)
{
    auto size = font.measureText(text);
    float x = (SCREEN_WIDTH - size.x) / 2;
    r.drawText(font, text, {x, y});
}

// Titre écran
drawCenteredText(renderer, fontManager.get("title"),
                 "R-TYPE", 100);
```

---

## Tailles Recommandées

| Usage | Police | Taille |
|-------|--------|--------|
| HUD (score, vie) | pixel | 24px |
| Menu items | pixel | 32px |
| Titres | title | 64px |
| Sous-titres | pixel_bold | 24px |
| Debug info | mono | 14px |
| Chat | pixel | 16px |

---

## Couleurs

### Palette Texte

| Contexte | Couleur | Hex |
|----------|---------|-----|
| Normal | Blanc | `#FFFFFF` |
| Sélectionné | Jaune | `#FFD700` |
| Désactivé | Gris | `#808080` |
| Erreur | Rouge | `#FF4444` |
| Succès | Vert | `#44FF44` |
| Info | Cyan | `#44FFFF` |

### Application

```cpp
enum class TextColor {
    Normal,
    Selected,
    Disabled,
    Error,
    Success,
    Info
};

Color getTextColor(TextColor type) {
    switch (type) {
        case TextColor::Normal:   return {255, 255, 255};
        case TextColor::Selected: return {255, 215, 0};
        case TextColor::Disabled: return {128, 128, 128};
        case TextColor::Error:    return {255, 68, 68};
        case TextColor::Success:  return {68, 255, 68};
        case TextColor::Info:     return {68, 255, 255};
    }
    return {255, 255, 255};
}
```

---

## Effets Texte

### Ombre

```cpp
void drawTextWithShadow(Renderer& r, const Font& font,
                        const std::string& text, Vector2f pos)
{
    // Ombre (décalée de 2px)
    r.drawText(font, text, {pos.x + 2, pos.y + 2}, Color::Black);
    // Texte principal
    r.drawText(font, text, pos, Color::White);
}
```

### Contour

```cpp
void drawTextWithOutline(Renderer& r, const Font& font,
                         const std::string& text, Vector2f pos)
{
    // Contour (4 directions)
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            if (dx == 0 && dy == 0) continue;
            r.drawText(font, text,
                       {pos.x + dx, pos.y + dy},
                       Color::Black);
        }
    }
    // Texte principal
    r.drawText(font, text, pos, Color::White);
}
```

### Animation (Pulsation)

```cpp
class PulsingText {
    float time_ = 0;

public:
    void update(float dt) {
        time_ += dt;
    }

    void render(Renderer& r, const Font& font,
                const std::string& text, Vector2f pos)
    {
        float scale = 1.0f + 0.1f * std::sin(time_ * 3.0f);
        r.drawText(font, text, pos, Color::White, scale);
    }
};
```

---

## Accessibilité

### Tailles Minimum

| Contexte | Taille Min |
|----------|------------|
| Corps de texte | 16px |
| Labels | 14px |
| Légendes | 12px |

### Contraste

- Ratio minimum: **4.5:1** pour texte normal
- Ratio minimum: **3:1** pour grand texte (24px+)

### Mode Daltonien

Ne pas utiliser uniquement la couleur pour transmettre l'information. Combiner avec:

- Icônes
- Formes
- Position

---

## Licences

| Police | Licence | Source |
|--------|---------|--------|
| Pixel | OFL 1.1 | [Google Fonts](https://fonts.google.com) |
| Title | OFL 1.1 | [Google Fonts](https://fonts.google.com) |
| Mono | OFL 1.1 | [JetBrains](https://www.jetbrains.com/mono) |

!!! note "Open Font License"
    Les polices OFL peuvent être utilisées, modifiées et redistribuées librement, y compris dans des projets commerciaux.
