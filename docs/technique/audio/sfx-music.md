---
tags:
  - technique
  - audio
---

# SFX & Musique

Gestion des effets sonores et de la musique.

## AudioManager

```cpp
class AudioManager {
public:
    static AudioManager& instance();

    // Chargement
    void loadSound(const std::string& name, const std::string& path);
    void loadMusic(const std::string& name, const std::string& path);

    // Lecture
    void playSound(const std::string& name, float volume = 1.0f);
    void playMusic(const std::string& name, bool loop = true);
    void stopMusic();
    void pauseMusic();
    void resumeMusic();

    // Volume
    void setMasterVolume(float vol);  // 0.0 - 1.0
    void setMusicVolume(float vol);
    void setSfxVolume(float vol);

private:
    std::map<std::string, std::unique_ptr<Sound>> sounds_;
    std::map<std::string, std::unique_ptr<Music>> music_;
    std::string currentMusic_;

    float masterVolume_ = 0.8f;
    float musicVolume_ = 0.6f;
    float sfxVolume_ = 1.0f;
};
```

---

## Formats Supportés

| Format | Extension | Usage |
|--------|-----------|-------|
| WAV | `.wav` | SFX courts |
| OGG | `.ogg` | Musique, SFX longs |
| FLAC | `.flac` | Musique haute qualité |
| MP3 | `.mp3` | Musique (lossy) |

---

## Implémentation SDL2

```cpp
#include <SDL2/SDL_mixer.h>

class SDL2AudioManager : public IAudioManager {
    std::map<std::string, Mix_Chunk*> sounds_;
    Mix_Music* currentMusic_ = nullptr;

public:
    SDL2AudioManager() {
        Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);
        Mix_AllocateChannels(16);
    }

    ~SDL2AudioManager() {
        for (auto& [name, chunk] : sounds_)
            Mix_FreeChunk(chunk);
        if (currentMusic_)
            Mix_FreeMusic(currentMusic_);
        Mix_CloseAudio();
    }

    void loadSound(const std::string& name, const std::string& path) override {
        sounds_[name] = Mix_LoadWAV(path.c_str());
    }

    void playSound(const std::string& name, float volume) override {
        if (auto it = sounds_.find(name); it != sounds_.end()) {
            int channel = Mix_PlayChannel(-1, it->second, 0);
            Mix_Volume(channel, static_cast<int>(volume * sfxVolume_ * masterVolume_ * 128));
        }
    }

    void playMusic(const std::string& path, bool loop) override {
        if (currentMusic_)
            Mix_FreeMusic(currentMusic_);
        currentMusic_ = Mix_LoadMUS(path.c_str());
        Mix_PlayMusic(currentMusic_, loop ? -1 : 1);
        Mix_VolumeMusic(static_cast<int>(musicVolume_ * masterVolume_ * 128));
    }
};
```

---

## Implémentation SFML

```cpp
#include <SFML/Audio.hpp>

class SFMLAudioManager : public IAudioManager {
    std::map<std::string, sf::SoundBuffer> buffers_;
    std::map<std::string, sf::Sound> sounds_;
    sf::Music music_;

public:
    void loadSound(const std::string& name, const std::string& path) override {
        buffers_[name].loadFromFile(path);
        sounds_[name].setBuffer(buffers_[name]);
    }

    void playSound(const std::string& name, float volume) override {
        if (auto it = sounds_.find(name); it != sounds_.end()) {
            it->second.setVolume(volume * sfxVolume_ * masterVolume_ * 100);
            it->second.play();
        }
    }

    void playMusic(const std::string& path, bool loop) override {
        music_.openFromFile(path);
        music_.setLoop(loop);
        music_.setVolume(musicVolume_ * masterVolume_ * 100);
        music_.play();
    }
};
```

---

## Sons du Jeu

| Son | Fichier | Événement |
|-----|---------|-----------|
| `shoot` | `shoot.wav` | Tir joueur |
| `explosion` | `explosion.wav` | Destruction |
| `hit` | `hit.wav` | Dégâts |
| `powerup` | `powerup.wav` | Bonus |
| `menu_select` | `select.wav` | Navigation menu |
| `game_over` | `gameover.wav` | Défaite |

---

## Musiques

| Piste | Fichier | Contexte |
|-------|---------|----------|
| `menu` | `menu.ogg` | Menu principal |
| `game` | `game.ogg` | En jeu |
| `boss` | `boss.ogg` | Combat boss |
| `victory` | `victory.ogg` | Victoire |

---

## Utilisation

```cpp
// Au démarrage
auto& audio = AudioManager::instance();
audio.loadSound("shoot", "assets/audio/shoot.wav");
audio.loadSound("explosion", "assets/audio/explosion.wav");

// Dans le jeu
void Player::shoot() {
    AudioManager::instance().playSound("shoot");
    // ... spawn missile
}

void Enemy::onDestroyed() {
    AudioManager::instance().playSound("explosion");
}

// Menu settings
void SettingsScene::onVolumeChanged(float vol) {
    AudioManager::instance().setMasterVolume(vol);
}
```
