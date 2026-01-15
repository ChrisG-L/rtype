---
tags:
  - technique
  - audio
  - voice-chat
---

# Voice Chat Technique

Implémentation détaillée du système voice chat.

## Stack Technique

```mermaid
flowchart LR
    Mic[🎤] --> PA[PortAudio]
    PA --> Opus[Opus Encode]
    Opus --> UDP[UDP 4243]
    UDP --> Net[Réseau]
    Net --> UDP2[UDP Recv]
    UDP2 --> Opus2[Opus Decode]
    Opus2 --> PA2[PortAudio]
    PA2 --> Spk[🔊]
```

---

## PortAudio

### Initialisation

```cpp
void VoiceChatManager::initPortAudio() {
    Pa_Initialize();

    // Input stream (micro)
    PaStreamParameters inputParams;
    inputParams.device = Pa_GetDefaultInputDevice();
    inputParams.channelCount = 1;
    inputParams.sampleFormat = paFloat32;
    inputParams.suggestedLatency = 0.010;  // 10ms
    inputParams.hostApiSpecificStreamInfo = nullptr;

    Pa_OpenStream(&inputStream_,
        &inputParams, nullptr,
        48000,          // Sample rate
        960,            // Frames per buffer (20ms)
        paClipOff,
        nullptr, nullptr);

    // Output stream (speakers)
    PaStreamParameters outputParams;
    outputParams.device = Pa_GetDefaultOutputDevice();
    outputParams.channelCount = 1;
    outputParams.sampleFormat = paFloat32;
    outputParams.suggestedLatency = 0.010;

    Pa_OpenStream(&outputStream_,
        nullptr, &outputParams,
        48000, 960, paClipOff,
        nullptr, nullptr);

    Pa_StartStream(inputStream_);
    Pa_StartStream(outputStream_);
}
```

### Capture

```cpp
void VoiceChatManager::captureFrame() {
    float samples[960];
    Pa_ReadStream(inputStream_, samples, 960);

    if (mode_ == VoiceMode::VAD && !detectVoice(samples, 960))
        return;

    if (mode_ == VoiceMode::PTT && !pttPressed_)
        return;

    // Encode and send
    uint8_t encoded[256];
    int len = opus_encode_float(encoder_, samples, 960, encoded, 256);

    VoicePacket packet{
        .sender_id = localId_,
        .sequence = ++sequence_,
        .opus_length = static_cast<uint16_t>(len)
    };
    std::memcpy(packet.opus_data, encoded, len);

    socket_.send(packet);
}
```

---

## Opus Codec

### Configuration

```cpp
void VoiceChatManager::initOpus() {
    int error;

    // Encoder
    encoder_ = opus_encoder_create(
        48000,                    // Sample rate
        1,                        // Channels (mono)
        OPUS_APPLICATION_VOIP,    // Optimisé pour voix
        &error
    );

    opus_encoder_ctl(encoder_, OPUS_SET_BITRATE(24000));
    opus_encoder_ctl(encoder_, OPUS_SET_COMPLEXITY(5));
    opus_encoder_ctl(encoder_, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));

    // Decoder
    decoder_ = opus_decoder_create(48000, 1, &error);
}
```

### Paramètres

| Paramètre | Valeur | Raison |
|-----------|--------|--------|
| Sample Rate | 48000 Hz | Qualité maximale Opus |
| Channels | 1 | Voix mono suffisant |
| Frame Size | 960 samples | 20ms @ 48kHz |
| Bitrate | 24 kbps | Qualité/bande passante |
| Complexity | 5 | Équilibre CPU/qualité |
| Application | VOIP | Optimisé voix |

---

## Voice Activity Detection

```cpp
bool VoiceChatManager::detectVoice(const float* samples, int count) {
    // Calcul de l'énergie RMS
    float energy = 0;
    for (int i = 0; i < count; i++) {
        energy += samples[i] * samples[i];
    }
    energy = std::sqrt(energy / count);

    // Hystérésis pour éviter le "pumping"
    if (energy > vadThreshold_ * 1.5f) {
        vadActive_ = true;
        vadHoldoff_ = 20;  // 20 frames = 400ms
    } else if (vadActive_ && vadHoldoff_ > 0) {
        vadHoldoff_--;
    } else {
        vadActive_ = false;
    }

    return vadActive_;
}
```

---

## Jitter Buffer

Gestion des variations de latence réseau.

```cpp
class JitterBuffer {
    static constexpr int BUFFER_MS = 60;  // 3 frames
    static constexpr int MAX_FRAMES = 10;

    struct BufferedFrame {
        uint32_t sequence;
        std::vector<float> samples;
        bool valid = false;
    };

    std::array<BufferedFrame, MAX_FRAMES> buffer_;
    uint32_t playSequence_ = 0;
    int bufferLevel_ = 0;

public:
    void push(uint32_t seq, const float* samples, int count) {
        int idx = seq % MAX_FRAMES;
        buffer_[idx].sequence = seq;
        buffer_[idx].samples.assign(samples, samples + count);
        buffer_[idx].valid = true;
        bufferLevel_++;
    }

    bool pop(float* out, int count) {
        // Attendre niveau minimum
        if (bufferLevel_ < 3) {
            std::fill(out, out + count, 0);
            return false;
        }

        int idx = playSequence_ % MAX_FRAMES;
        if (buffer_[idx].valid && buffer_[idx].sequence == playSequence_) {
            std::copy(buffer_[idx].samples.begin(),
                      buffer_[idx].samples.end(), out);
            buffer_[idx].valid = false;
            bufferLevel_--;
        } else {
            // Packet perdu - interpolation ou silence
            std::fill(out, out + count, 0);
        }

        playSequence_++;
        return true;
    }
};
```

---

## Latence Totale

| Composant | Latence |
|-----------|---------|
| Capture PortAudio | 10 ms |
| Frame buffer | 20 ms |
| Opus encode | < 5 ms |
| Network | Variable |
| Jitter buffer | 60 ms |
| Opus decode | < 5 ms |
| Playback | 10 ms |
| **Total** | ~110 ms + network |
