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
assets/
└── fonts/
    ├── ARIA.TTF      # Police principale
    └── aria.ttf      # Copie (casse différente)
```

---

## Police Utilisée

### ARIA

| Propriété | Valeur |
|-----------|--------|
| Fichier | `ARIA.TTF` |
| Format | TrueType Font |
| Foundry | Monotype Corporation |
| Usage | Toute l'interface (menus, HUD, textes) |

**Caractéristiques :**
- Police sans-serif lisible
- Support Unicode complet
- Convient pour l'affichage à diverses tailles

---

## Utilisation dans le Code

### Chargement

Toutes les scènes chargent la police de la même façon :

```cpp
// Constante commune
static constexpr const char* FONT_KEY = "main";

// Chargement
_context.window->loadFont(FONT_KEY, "assets/fonts/ARIA.TTF");
```

### Scènes Utilisant la Police

| Scène | Fichier |
|-------|---------|
| GameScene | `src/client/src/scenes/GameScene.cpp:61` |
| LoginScene | `src/client/src/scenes/LoginScene.cpp:28` |
| MainMenuScene | `src/client/src/scenes/MainMenuScene.cpp:25` |
| LobbyScene | `src/client/src/scenes/LobbyScene.cpp:34` |
| RoomBrowserScene | `src/client/src/scenes/RoomBrowserScene.cpp:22` |
| SettingsScene | `src/client/src/scenes/SettingsScene.cpp:88` |
| ConnectionScene | `src/client/src/scenes/ConnectionScene.cpp:30` |

### Rendu de Texte

```cpp
// Signature de drawText dans IWindow
virtual void drawText(
    const std::string& fontKey,
    const std::string& text,
    float x, float y,
    unsigned int size,
    rgba color
) = 0;

// Exemple d'utilisation
_context.window->drawText(
    FONT_KEY,
    "Score: " + std::to_string(score),
    10.0f, 10.0f,  // position
    24,            // taille en pixels
    {255, 255, 255, 255}  // couleur RGBA
);
```

---

## Tailles Recommandées

| Usage | Taille |
|-------|--------|
| HUD (score, vie) | 24px |
| Menu items | 32px |
| Titres | 48px |
| Sous-titres | 24px |
| Chat | 16px |
| Debug info | 14px |

---

## Couleurs

### Palette Texte Courante

| Contexte | Couleur | RGBA |
|----------|---------|------|
| Normal | Blanc | `{255, 255, 255, 255}` |
| Sélectionné | Jaune | `{255, 215, 0, 255}` |
| Désactivé | Gris | `{128, 128, 128, 255}` |
| Erreur | Rouge | `{255, 68, 68, 255}` |
| Succès | Vert | `{68, 255, 68, 255}` |
| Info | Cyan | `{68, 255, 255, 255}` |

### Exemple

```cpp
// Couleur normale (blanc)
rgba white = {255, 255, 255, 255};
_context.window->drawText(FONT_KEY, "Score: 1000", 10, 10, 24, white);

// Couleur erreur (rouge)
rgba red = {255, 68, 68, 255};
_context.window->drawText(FONT_KEY, "Connection failed!", 10, 50, 24, red);
```

---

## Effets Texte

### Ombre

```cpp
void drawTextWithShadow(
    graphics::IWindow& window,
    const std::string& text,
    float x, float y,
    unsigned int size,
    rgba color)
{
    // Ombre (décalée de 2px, noire)
    window.drawText(FONT_KEY, text, x + 2, y + 2, size, {0, 0, 0, 200});
    // Texte principal
    window.drawText(FONT_KEY, text, x, y, size, color);
}
```

### Contour

```cpp
void drawTextWithOutline(
    graphics::IWindow& window,
    const std::string& text,
    float x, float y,
    unsigned int size,
    rgba color)
{
    rgba black = {0, 0, 0, 255};

    // Contour (4 directions)
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            if (dx == 0 && dy == 0) continue;
            window.drawText(FONT_KEY, text, x + dx, y + dy, size, black);
        }
    }
    // Texte principal
    window.drawText(FONT_KEY, text, x, y, size, color);
}
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

- Ratio minimum : **4.5:1** pour texte normal
- Ratio minimum : **3:1** pour grand texte (24px+)

### Recommandations

- Ne pas utiliser uniquement la couleur pour transmettre l'information
- Combiner couleur + icônes/formes quand possible
- Éviter le texte trop petit (<12px)

---

## Ajout de Nouvelles Polices

Pour ajouter une nouvelle police :

1. Placer le fichier `.ttf` ou `.otf` dans `assets/fonts/`
2. Charger avec `loadFont()` :

```cpp
_context.window->loadFont("nouvelle_police", "assets/fonts/MaPolice.ttf");
```

### Formats Supportés

| Format | Extension | Support |
|--------|-----------|---------|
| TrueType | `.ttf` | Oui |
| OpenType | `.otf` | Oui |

---

## Notes

!!! note "Casse du Fichier"
    Deux fichiers existent : `ARIA.TTF` et `aria.ttf`.
    Le code utilise `ARIA.TTF` (majuscules).
    Sur Linux (case-sensitive), assurez-vous d'utiliser la bonne casse.
