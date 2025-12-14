/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** AudioManager - Implementation
*/

#include "audio/AudioManager.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <iostream>

namespace audio {

AudioManager& AudioManager::getInstance()
{
    static AudioManager instance;
    return instance;
}

AudioManager::~AudioManager()
{
    shutdown();
}

bool AudioManager::init()
{
    if (_initialized) {
        return true;
    }

    if (SDL_WasInit(SDL_INIT_AUDIO) == 0) {
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
            std::cerr << "Failed to init SDL audio: " << SDL_GetError() << std::endl;
            return false;
        }
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "Failed to init SDL_mixer: " << Mix_GetError() << std::endl;
        return false;
    }

    Mix_AllocateChannels(16);

    _initialized = true;
    std::cout << "Audio system initialized" << std::endl;
    return true;
}

void AudioManager::shutdown()
{
    if (!_initialized) {
        return;
    }

    unloadAll();

    Mix_CloseAudio();
    _initialized = false;
    std::cout << "Audio system shut down" << std::endl;
}

bool AudioManager::loadMusic(const std::string& key, const std::string& filepath)
{
    if (!_initialized) {
        std::cerr << "Audio not initialized, cannot load music: " << filepath << std::endl;
        return false;
    }

    auto it = _music.find(key);
    if (it != _music.end()) {
        Mix_FreeMusic(it->second);
        _music.erase(it);
    }

    Mix_Music* music = Mix_LoadMUS(filepath.c_str());
    if (!music) {
        std::cerr << "Failed to load music '" << filepath << "': " << Mix_GetError() << std::endl;
        return false;
    }

    _music[key] = music;
    std::cout << "Loaded music: " << key << " from " << filepath << std::endl;
    return true;
}

void AudioManager::playMusic(const std::string& key, int loops)
{
    if (!_initialized || _muted) {
        return;
    }

    auto it = _music.find(key);
    if (it == _music.end()) {
        std::cerr << "Music not found: " << key << std::endl;
        return;
    }

    Mix_VolumeMusic(calculateEffectiveVolume(_musicVolume) * MIX_MAX_VOLUME / 100);

    if (Mix_PlayMusic(it->second, loops) < 0) {
        std::cerr << "Failed to play music: " << Mix_GetError() << std::endl;
        return;
    }

    _currentMusicKey = key;
}

void AudioManager::stopMusic()
{
    Mix_HaltMusic();
    _currentMusicKey.clear();
}

void AudioManager::pauseMusic()
{
    Mix_PauseMusic();
}

void AudioManager::resumeMusic()
{
    if (!_muted) {
        Mix_ResumeMusic();
    }
}

bool AudioManager::isMusicPlaying() const
{
    return Mix_PlayingMusic() != 0;
}

void AudioManager::setMusicVolume(int volume)
{
    _musicVolume = std::max(0, std::min(100, volume));
    if (_initialized && !_muted) {
        Mix_VolumeMusic(calculateEffectiveVolume(_musicVolume) * MIX_MAX_VOLUME / 100);
    }
}

int AudioManager::getMusicVolume() const
{
    return _musicVolume;
}

bool AudioManager::loadSound(const std::string& key, const std::string& filepath)
{
    if (!_initialized) {
        std::cerr << "Audio not initialized, cannot load sound: " << filepath << std::endl;
        return false;
    }

    auto it = _sounds.find(key);
    if (it != _sounds.end()) {
        Mix_FreeChunk(it->second);
        _sounds.erase(it);
    }

    Mix_Chunk* chunk = Mix_LoadWAV(filepath.c_str());
    if (!chunk) {
        std::cerr << "Failed to load sound '" << filepath << "': " << Mix_GetError() << std::endl;
        return false;
    }

    _sounds[key] = chunk;
    std::cout << "Loaded sound: " << key << " from " << filepath << std::endl;
    return true;
}

int AudioManager::playSound(const std::string& key, int loops)
{
    if (!_initialized || _muted) {
        return -1;
    }

    auto it = _sounds.find(key);
    if (it == _sounds.end()) {
        return -1;
    }

    Mix_VolumeChunk(it->second, calculateEffectiveVolume(_soundVolume) * MIX_MAX_VOLUME / 100);

    int channel = Mix_PlayChannel(-1, it->second, loops);
    if (channel < 0) {
        return -1;
    }

    return channel;
}

void AudioManager::setSoundVolume(int volume)
{
    _soundVolume = std::max(0, std::min(100, volume));
}

int AudioManager::getSoundVolume() const
{
    return _soundVolume;
}

void AudioManager::setMasterVolume(int volume)
{
    _masterVolume = std::max(0, std::min(100, volume));

    if (_initialized && !_muted) {
        Mix_VolumeMusic(calculateEffectiveVolume(_musicVolume) * MIX_MAX_VOLUME / 100);
    }
}

void AudioManager::setMuted(bool muted)
{
    _muted = muted;

    if (_initialized) {
        if (muted) {
            Mix_VolumeMusic(0);
        } else {
            Mix_VolumeMusic(calculateEffectiveVolume(_musicVolume) * MIX_MAX_VOLUME / 100);
        }
    }
}

void AudioManager::unloadAll()
{
    Mix_HaltMusic();
    Mix_HaltChannel(-1);

    for (auto& [key, music] : _music) {
        if (music) {
            Mix_FreeMusic(music);
        }
    }
    _music.clear();

    for (auto& [key, chunk] : _sounds) {
        if (chunk) {
            Mix_FreeChunk(chunk);
        }
    }
    _sounds.clear();

    _currentMusicKey.clear();
}

int AudioManager::calculateEffectiveVolume(int volume) const
{
    return (volume * _masterVolume) / 100;
}

}
