/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** AudioManager - Simple audio system for music and sound effects
*/

#ifndef AUDIOMANAGER_HPP_
#define AUDIOMANAGER_HPP_

#include <string>
#include <unordered_map>
#include <memory>
#include <cstdint>
#include <SDL2/SDL_mixer.h>

namespace audio {

/**
 * @brief Simple audio manager for background music and sound effects
 *
 * Usage:
 *   auto& audio = AudioManager::getInstance();
 *   audio.init();
 *   audio.loadMusic("bgm", "assets/audio/background.ogg");
 *   audio.loadSound("shoot", "assets/audio/shoot.wav");
 *   audio.playMusic("bgm");
 *   audio.playSound("shoot");
 */
class AudioManager {
public:
    static AudioManager& getInstance();

    /**
     * @brief Initialize the audio system
     * @return true if initialization succeeded
     */
    bool init();

    /**
     * @brief Shutdown the audio system and free resources
     */
    void shutdown();

    /**
     * @brief Check if audio system is initialized
     */
    bool isInitialized() const { return _initialized; }

    /**
     * @brief Load a music file (for background music)
     * @param key Identifier for this music
     * @param filepath Path to audio file (.ogg, .mp3, .wav, .flac)
     * @return true if loaded successfully
     */
    bool loadMusic(const std::string& key, const std::string& filepath);

    /**
     * @brief Play background music (loops indefinitely)
     * @param key Music identifier
     * @param loops Number of loops (-1 for infinite, 0 for once)
     */
    void playMusic(const std::string& key, int loops = -1);

    /**
     * @brief Stop current background music
     */
    void stopMusic();

    /**
     * @brief Pause current background music
     */
    void pauseMusic();

    /**
     * @brief Resume paused background music
     */
    void resumeMusic();

    /**
     * @brief Check if music is currently playing
     */
    bool isMusicPlaying() const;

    /**
     * @brief Set music volume
     * @param volume 0-100
     */
    void setMusicVolume(int volume);

    /**
     * @brief Get current music volume
     * @return Volume 0-100
     */
    int getMusicVolume() const;

    /**
     * @brief Load a sound effect
     * @param key Identifier for this sound
     * @param filepath Path to audio file (.wav recommended for effects)
     * @return true if loaded successfully
     */
    bool loadSound(const std::string& key, const std::string& filepath);

    /**
     * @brief Play a sound effect
     * @param key Sound identifier
     * @param loops Number of loops (0 = play once)
     * @return Channel number the sound is playing on, -1 on error
     */
    int playSound(const std::string& key, int loops = 0);

    /**
     * @brief Set sound effects volume
     * @param volume 0-100
     */
    void setSoundVolume(int volume);

    /**
     * @brief Get current sound effects volume
     * @return Volume 0-100
     */
    int getSoundVolume() const;

    /**
     * @brief Set master volume (affects both music and sounds)
     * @param volume 0-100
     */
    void setMasterVolume(int volume);

    /**
     * @brief Mute/unmute all audio
     */
    void setMuted(bool muted);

    /**
     * @brief Check if audio is muted
     */
    bool isMuted() const { return _muted; }

    /**
     * @brief Unload all loaded audio resources
     */
    void unloadAll();

private:
    AudioManager() = default;
    ~AudioManager();
    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    bool _initialized = false;
    bool _muted = false;
    int _musicVolume = 70;
    int _soundVolume = 80;
    int _masterVolume = 100;

    std::unordered_map<std::string, Mix_Music*> _music;
    std::unordered_map<std::string, Mix_Chunk*> _sounds;

    std::string _currentMusicKey;

    int calculateEffectiveVolume(int volume) const;
};

}

#endif /* !AUDIOMANAGER_HPP_ */
