---
tags:
  - technique
  - graphiques
  - shaders
---

# Shaders et Post-Processing

Effets visuels et filtres d'accessibilité.

## Architecture

```mermaid
flowchart LR
    subgraph Render Pipeline
        Scene[Scene Render] --> FB[Framebuffer]
        FB --> PP[Post-Process]
        PP --> Screen[Display]
    end

    subgraph Shaders
        Bloom[Bloom]
        CB[Colorblind]
        Vignette[Vignette]
    end

    PP --> Bloom
    PP --> CB
    PP --> Vignette

    style PP fill:#7c3aed,color:#fff
```

---

## Post-Processing Manager

```cpp
class PostProcessor {
    GLuint framebuffer_;
    GLuint colorTexture_;
    GLuint quadVAO_;

    std::vector<Shader*> effects_;
    bool colorblindEnabled_ = false;
    ColorblindMode colorblindMode_ = ColorblindMode::Normal;

public:
    void init(int width, int height) {
        // Create framebuffer
        glGenFramebuffers(1, &framebuffer_);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);

        // Color attachment
        glGenTextures(1, &colorTexture_);
        glBindTexture(GL_TEXTURE_2D, colorTexture_);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height,
                     0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, colorTexture_, 0);

        createQuad();
    }

    void beginScene() {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    void endScene() {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Apply effects chain
        for (auto* effect : effects_) {
            applyEffect(effect);
        }

        // Final render to screen
        renderToScreen();
    }
};
```

---

## Filtres Daltonisme

### Modes Supportés

| Mode | Description | Prévalence |
|------|-------------|------------|
| `Protanopia` | Rouge absent | 1% hommes |
| `Deuteranopia` | Vert absent | 1% hommes |
| `Tritanopia` | Bleu absent | 0.01% |
| `Achromatopsia` | Noir/blanc | Rare |
| `HighContrast` | Contraste élevé | - |

### Shader Daltonisme

```glsl
#version 330 core

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D screenTexture;
uniform int colorblindMode;

// Matrices de transformation
const mat3 PROTANOPIA = mat3(
    0.567, 0.433, 0.000,
    0.558, 0.442, 0.000,
    0.000, 0.242, 0.758
);

const mat3 DEUTERANOPIA = mat3(
    0.625, 0.375, 0.000,
    0.700, 0.300, 0.000,
    0.000, 0.300, 0.700
);

const mat3 TRITANOPIA = mat3(
    0.950, 0.050, 0.000,
    0.000, 0.433, 0.567,
    0.000, 0.475, 0.525
);

void main() {
    vec3 color = texture(screenTexture, TexCoords).rgb;

    switch (colorblindMode) {
        case 1: // Protanopia
            color = PROTANOPIA * color;
            break;
        case 2: // Deuteranopia
            color = DEUTERANOPIA * color;
            break;
        case 3: // Tritanopia
            color = TRITANOPIA * color;
            break;
        case 4: // Achromatopsia
            float gray = dot(color, vec3(0.299, 0.587, 0.114));
            color = vec3(gray);
            break;
        case 5: // High Contrast
            color = pow(color, vec3(0.7)) * 1.2;
            color = clamp(color, 0.0, 1.0);
            break;
    }

    FragColor = vec4(color, 1.0);
}
```

---

## Effets Visuels

### Bloom (Lueur)

```glsl
// bloom_extract.frag - Extraction des zones lumineuses
#version 330 core

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D scene;
uniform float threshold;

void main() {
    vec3 color = texture(scene, TexCoords).rgb;
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));

    if (brightness > threshold) {
        FragColor = vec4(color, 1.0);
    } else {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}

// bloom_blur.frag - Flou gaussien
#version 330 core

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D image;
uniform bool horizontal;

const float weights[5] = float[](
    0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216
);

void main() {
    vec2 texOffset = 1.0 / textureSize(image, 0);
    vec3 result = texture(image, TexCoords).rgb * weights[0];

    for (int i = 1; i < 5; i++) {
        vec2 offset = horizontal
            ? vec2(texOffset.x * i, 0.0)
            : vec2(0.0, texOffset.y * i);

        result += texture(image, TexCoords + offset).rgb * weights[i];
        result += texture(image, TexCoords - offset).rgb * weights[i];
    }

    FragColor = vec4(result, 1.0);
}
```

### Vignette

```glsl
#version 330 core

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D scene;
uniform float intensity;
uniform float radius;

void main() {
    vec3 color = texture(scene, TexCoords).rgb;

    vec2 center = TexCoords - vec2(0.5);
    float dist = length(center);
    float vignette = smoothstep(radius, radius - 0.2, dist);

    color *= mix(1.0, vignette, intensity);

    FragColor = vec4(color, 1.0);
}
```

---

## Intégration SDL2/SFML

### Avec SDL2 + OpenGL

```cpp
class SDL2PostProcessor : public PostProcessor {
    SDL_GLContext glContext_;

public:
    void init(SDL_Window* window) {
        glContext_ = SDL_GL_CreateContext(window);

        // Load shaders
        loadShader("bloom", "shaders/bloom.vert", "shaders/bloom.frag");
        loadShader("colorblind", "shaders/colorblind.vert",
                   "shaders/colorblind.frag");

        PostProcessor::init(1920, 1080);
    }
};
```

### Avec SFML

```cpp
class SFMLPostProcessor {
    sf::Shader colorblindShader_;
    sf::RenderTexture renderTexture_;

public:
    void init() {
        renderTexture_.create(1920, 1080);

        colorblindShader_.loadFromFile(
            "shaders/colorblind.frag",
            sf::Shader::Fragment
        );
    }

    void setColorblindMode(ColorblindMode mode) {
        colorblindShader_.setUniform("colorblindMode",
                                     static_cast<int>(mode));
    }

    void apply(sf::RenderTarget& target) {
        sf::Sprite sprite(renderTexture_.getTexture());
        target.draw(sprite, &colorblindShader_);
    }
};
```

---

## Configuration

```json
{
  "graphics": {
    "postProcessing": {
      "enabled": true,
      "bloom": {
        "enabled": true,
        "threshold": 0.8,
        "intensity": 1.0
      },
      "vignette": {
        "enabled": false,
        "intensity": 0.3,
        "radius": 0.8
      }
    },
    "accessibility": {
      "colorblindMode": "none"
    }
  }
}
```

---

## Pipeline Complet

```mermaid
sequenceDiagram
    participant Game as Game Loop
    participant PP as PostProcessor
    participant FB as Framebuffer
    participant Shader as Shaders
    participant Screen as Display

    Game->>PP: beginScene()
    PP->>FB: Bind framebuffer

    Note over Game: Render scene...

    Game->>PP: endScene()
    PP->>Shader: Extract bright pixels
    PP->>Shader: Blur (horizontal)
    PP->>Shader: Blur (vertical)
    PP->>Shader: Combine bloom
    PP->>Shader: Apply colorblind
    PP->>Screen: Final render
```

---

## Paramètres

| Paramètre | Défaut | Description |
|-----------|--------|-------------|
| `bloom.threshold` | 0.8 | Seuil luminosité |
| `bloom.intensity` | 1.0 | Force du bloom |
| `vignette.intensity` | 0.3 | Assombrissement bords |
| `vignette.radius` | 0.8 | Rayon central |
