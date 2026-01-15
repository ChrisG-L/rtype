---
tags:
  - assets
  - audio
  - sons
---

# Sons et Musiques

Ressources audio du jeu R-Type.

## Structure

```
assets/
└── audio/
    ├── shoot.mp3        # Son de tir
    ├── hit.mp3          # Son d'impact
    ├── background.ogg   # Musique de fond (optionnel)
    └── README.md        # Documentation des assets audio
```

---

## Fichiers Actuels

| Fichier | Format | Type | Description |
|---------|--------|------|-------------|
| `shoot.mp3` | MP3 | SFX | Son de tir du joueur |
| `hit.mp3` | MP3 | SFX | Son d'impact (ennemi touché) |

!!! warning "Fichiers Manquants"
    Le fichier `background.ogg` est référencé dans le code mais n'existe pas encore dans le dépôt.
    Voir `assets/audio/README.md` pour la liste des fichiers recommandés.

---

## Formats Supportés

Le système audio utilise **SDL2_mixer**.

### Musique (Background)

| Format | Extension | Recommandé |
|--------|-----------|------------|
| OGG Vorbis | `.ogg` | Oui |
| MP3 | `.mp3` | Oui |
| FLAC | `.flac` | Non (taille) |
| WAV | `.wav` | Non (taille) |

### Effets Sonores

| Format | Extension | Recommandé |
|--------|-----------|------------|
| WAV | `.wav` | Oui (faible latence) |
| MP3 | `.mp3` | Oui |
| OGG | `.ogg` | Oui |

---

## Utilisation dans le Code

### AudioManager (Singleton)

```cpp
#include "audio/AudioManager.hpp"

// Récupérer l'instance
auto& audio = audio::AudioManager::getInstance();

// Initialiser le système audio
audio.init();
```

### Chargement et Lecture

```cpp
// Effets sonores
audio.loadSound("shoot", "assets/audio/shoot.mp3");
audio.loadSound("hit", "assets/audio/hit.mp3");

// Musique de fond
audio.loadMusic("bgm", "assets/audio/background.ogg");

// Jouer un effet sonore
audio.playSound("shoot");

// Jouer la musique en boucle
audio.playMusic("bgm", -1);  // -1 = boucle infinie
```

### Exemple dans GameScene

```cpp
// GameScene.cpp

void GameScene::loadAudio() {
    auto& audioMgr = audio::AudioManager::getInstance();
    audioMgr.loadMusic("bgm", "assets/audio/background.ogg");
    audioMgr.loadSound("shoot", "assets/audio/shoot.mp3");
    audioMgr.loadSound("hit", "assets/audio/hit.mp3");
}

void GameScene::onEnter() {
    auto& audioMgr = audio::AudioManager::getInstance();
    audioMgr.playMusic("bgm", -1);  // Musique en boucle
}

void GameScene::shootMissile() {
    auto& audioMgr = audio::AudioManager::getInstance();
    audioMgr.playSound("shoot");
    // ... logique de tir
}
```

---

## Contrôle du Volume

### Volumes par Défaut

| Canal | Volume | Description |
|-------|--------|-------------|
| Master | 100% | Volume principal |
| Music | 70% | Musique de fond |
| Sound | 80% | Effets sonores |

### API de Volume

```cpp
auto& audio = audio::AudioManager::getInstance();

// Volume master (affecte tout)
audio.setMasterVolume(100);  // 0-100

// Volume musique
audio.setMusicVolume(70);    // 0-100
int musicVol = audio.getMusicVolume();

// Volume effets sonores
audio.setSoundVolume(80);    // 0-100
int soundVol = audio.getSoundVolume();

// Mute/Unmute
audio.setMuted(true);
bool muted = audio.isMuted();
```

---

## Contrôle de la Musique

```cpp
auto& audio = audio::AudioManager::getInstance();

// Lecture
audio.playMusic("bgm", -1);  // -1 = infini, 0 = une fois
audio.playMusic("bgm", 3);   // Jouer 3 fois

// Contrôles
audio.pauseMusic();
audio.resumeMusic();
audio.stopMusic();

// État
bool playing = audio.isMusicPlaying();
```

---

## Fichiers Recommandés

Voir `assets/audio/README.md` pour la liste complète :

| Fichier | Type | Description |
|---------|------|-------------|
| `background.ogg` | Music | Musique de fond (loops) |
| `shoot.wav` | SFX | Tir joueur |
| `explosion.wav` | SFX | Destruction ennemi/missile |
| `hit.wav` | SFX | Impact sur joueur |
| `enemy_shoot.wav` | SFX | Tir ennemi |
| `player_death.wav` | SFX | Mort du joueur |

---

## Spécifications Techniques

### Effets Sonores

```
Format recommandé:
  - Type:       WAV PCM ou MP3
  - Canaux:     Mono (1 channel)
  - Fréquence:  44100 Hz
  - Bits:       16-bit
  - Durée:      < 2 secondes
```

### Musique

```
Format recommandé:
  - Type:       OGG Vorbis
  - Canaux:     Stéréo (2 channels)
  - Fréquence:  44100-48000 Hz
  - Bitrate:    128-192 kbps VBR
```

---

## Ressources Gratuites

Sites recommandés pour les assets audio :

- [OpenGameArt.org](https://opengameart.org/)
- [Freesound.org](https://freesound.org/)
- [Kenney.nl](https://kenney.nl/assets?q=audio)

---

## Conversion avec FFmpeg

```bash
# Convertir en WAV mono 44.1kHz (pour SFX)
ffmpeg -i input.mp3 -ac 1 -ar 44100 output.wav

# Convertir en OGG (pour musique)
ffmpeg -i input.wav -c:a libvorbis -q:a 5 output.ogg

# Convertir en MP3 (alternatif)
ffmpeg -i input.wav -c:a libmp3lame -q:a 2 output.mp3

# Normaliser le volume
ffmpeg -i input.wav -af "loudnorm=I=-16" output.wav
```
