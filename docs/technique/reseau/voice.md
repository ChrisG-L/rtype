---
tags:
  - technique
  - reseau
  - voice
---

# Protocole Voice

Communication audio temps réel avec **Opus** codec.

## Port

`4126/UDP`

---

## Technologies

| Composant | Technologie |
|-----------|-------------|
| **Codec** | Opus |
| **Capture** | PortAudio |
| **Transport** | UDP |

---

## Types de Messages

| Type | Value | Direction | Description |
|------|-------|-----------|-------------|
| `VoiceJoin` | `0x0300` | C→S | Rejoindre voice channel |
| `VoiceJoinAck` | `0x0301` | S→C | Confirmation |
| `VoiceLeave` | `0x0302` | C→S | Quitter voice |
| `VoiceFrame` | `0x0303` | Both | Audio Opus encodé |
| `VoiceMute` | `0x0304` | Both | Mute/unmute |

---

## Structures

### VoiceJoin (38 bytes)

```cpp
struct VoiceJoin {
    SessionToken token;      // 32 bytes - Réutilise l'auth TCP
    char roomCode[6];        // 6 bytes
};
```

### VoiceFrame (5-485 bytes)

```cpp
struct VoiceFrame {
    uint8_t  speaker_id;    // 1 byte - Qui parle
    uint16_t sequence;      // 2 bytes - Détection perte paquets
    uint16_t opus_len;      // 2 bytes - Taille données Opus
    uint8_t  opus_data[480]; // Max 480 bytes Opus
    // Header: 5 bytes, Total max: 485 bytes
};
```

### VoiceMute (2 bytes)

```cpp
struct VoiceMute {
    uint8_t player_id;  // 1 byte
    uint8_t muted;      // 1 byte - 0 = unmuted, 1 = muted
};
```

---

## Paramètres Opus

| Paramètre | Valeur | Description |
|-----------|--------|-------------|
| **Sample Rate** | 48000 Hz | Standard Opus |
| **Channels** | 1 (mono) | Voix uniquement |
| **Frame Size** | 960 samples | 20ms à 48kHz |
| **Bitrate** | 32000 bps | Qualité VoIP |
| **Max Frame Size** | 480 bytes | Taille max encodée |

---

## Architecture

```mermaid
flowchart LR
    subgraph Client Émetteur
        Mic[🎤 Micro] --> PA1[PortAudio]
        PA1 --> Enc[Opus Encode]
        Enc --> UDP1[UDP Send]
    end

    UDP1 --> Server
    Server --> UDP2[UDP Recv]

    subgraph Client Récepteur
        UDP2 --> Dec[Opus Decode]
        Dec --> PA2[PortAudio]
        PA2 --> Spk[🔊 Speaker]
    end
```

---

## Implémentation

### VoiceChatManager

```cpp
class VoiceChatManager {
    OpusEncoder* encoder_;
    OpusDecoder* decoder_;
    PaStream* inputStream_;
    PaStream* outputStream_;
    UDPSocket& socket_;

    std::queue<VoiceDataPacket> incoming_;
    bool muted_ = false;

public:
    VoiceChatManager(UDPSocket& socket)
        : socket_(socket)
    {
        // Init Opus
        int error;
        encoder_ = opus_encoder_create(48000, 1,
            OPUS_APPLICATION_VOIP, &error);
        decoder_ = opus_decoder_create(48000, 1, &error);

        opus_encoder_ctl(encoder_,
            OPUS_SET_BITRATE(32000));

        // Init PortAudio
        Pa_Initialize();
        setupStreams();
    }

    void capture() {
        if (muted_) return;

        float samples[960];
        Pa_ReadStream(inputStream_, samples, 960);

        // Encode
        uint8_t encoded[256];
        int len = opus_encode_float(encoder_,
            samples, 960, encoded, 256);

        // Send
        VoiceDataPacket packet{
            .sender_id = localPlayerId_,
            .sequence = ++sequence_,
            .timestamp = now(),
            .opus_length = static_cast<uint16_t>(len)
        };
        std::memcpy(packet.opus_data, encoded, len);

        socket_.broadcast(packet);
    }

    void playback() {
        while (!incoming_.empty()) {
            auto& packet = incoming_.front();

            // Decode
            float samples[960];
            opus_decode_float(decoder_,
                packet.opus_data, packet.opus_length,
                samples, 960, 0);

            // Play
            Pa_WriteStream(outputStream_, samples, 960);

            incoming_.pop();
        }
    }
};
```

---

## Push-to-Talk vs VAD

### PTT

```cpp
void VoiceChatManager::setPTT(bool pressed) {
    if (pressed && !muted_) {
        capturing_ = true;
    } else {
        capturing_ = false;
    }
}
```

### VAD (Voice Activity Detection)

```cpp
bool VoiceChatManager::detectVoice(float* samples, int count) {
    float energy = 0;
    for (int i = 0; i < count; i++) {
        energy += samples[i] * samples[i];
    }
    energy /= count;

    return energy > vadThreshold_;
}
```

---

## Jitter Buffer

Pour gérer les variations de latence :

```cpp
class JitterBuffer {
    std::map<uint32_t, VoiceDataPacket> buffer_;
    uint32_t playbackSeq_ = 0;
    static constexpr int BUFFER_SIZE = 3;  // 60ms

public:
    void push(VoiceDataPacket packet) {
        buffer_[packet.sequence] = std::move(packet);
    }

    std::optional<VoiceDataPacket> pop() {
        // Wait until we have enough buffer
        if (buffer_.size() < BUFFER_SIZE)
            return std::nullopt;

        auto it = buffer_.find(playbackSeq_++);
        if (it != buffer_.end()) {
            auto packet = std::move(it->second);
            buffer_.erase(it);
            return packet;
        }

        // Packet lost - generate silence or interpolate
        return generateSilence();
    }
};
```

---

## Latence

| Composant | Latence |
|-----------|---------|
| Capture | ~10ms |
| Encoding | ~20ms |
| Network | Variable |
| Jitter buffer | ~60ms |
| Decoding | ~20ms |
| Playback | ~10ms |
| **Total** | ~120ms + network |
