# Audio Assets

Place your audio files in this directory for the R-Type game.

## Required Files

| Filename | Type | Description | Recommended Format |
|----------|------|-------------|-------------------|
| `background.ogg` | Music | Main background music (loops) | OGG Vorbis |
| `shoot.wav` | SFX | Player shooting sound | WAV |
| `explosion.wav` | SFX | Explosion (enemy/missile destruction) | WAV |
| `hit.wav` | SFX | Player hit by enemy | WAV |
| `enemy_shoot.wav` | SFX | Enemy shooting sound | WAV |
| `player_death.wav` | SFX | Player death sound | WAV |

## Supported Formats

### Music (Background)
- **OGG Vorbis** (.ogg) - Recommended for music
- **MP3** (.mp3)
- **FLAC** (.flac)
- **WAV** (.wav) - Large file size, not recommended

### Sound Effects
- **WAV** (.wav) - Recommended for sound effects
- **OGG Vorbis** (.ogg)

## Tips

1. **Music**: Use OGG format for smaller file sizes with good quality
2. **Sound Effects**: Use WAV for instant playback (no decoding overhead)
3. **Sample Rate**: 44100 Hz is recommended
4. **Channels**: Stereo (2 channels) for music, Mono (1 channel) for effects
5. **Keep effects short**: Under 2 seconds for responsive gameplay

## Free Resources

You can find free game audio at:
- [OpenGameArt.org](https://opengameart.org/)
- [Freesound.org](https://freesound.org/)
- [Kenney.nl](https://kenney.nl/assets?q=audio)

## Volume Control

The game supports volume control via code:

```cpp
auto& audio = audio::AudioManager::getInstance();
audio.setMusicVolume(70);    // 0-100
audio.setSoundVolume(80);    // 0-100
audio.setMasterVolume(100);  // 0-100
audio.setMuted(false);       // Mute/unmute all
```
