# R-Type UDP Bandwidth Analysis & Benchmark

> **Protocol Version:** UDP Binary (Big-Endian)
> **Server Tick Rate:** 20 Hz (50ms)
> **Max Players:** 4

---

## Table of Contents

1. [Message Sizes](#1-message-sizes-wire-format)
2. [GameSnapshot Calculation](#2-gamesnapshot-size-calculation)
3. [Bandwidth Analysis](#3-bandwidth-analysis)
4. [RTT & Frame Timing](#4-rtt--frame-timing-analysis)
5. [Industry Comparison](#5-benchmark-comparisons)
6. [Stress Test](#6-worst-case-stress-test)
7. [Latency Recommendations](#7-latency-compensation-recommendations)
8. [Summary](#8-summary)

---

## 1. Message Sizes (Wire Format)

### UDP Header

| Component | Size |
|-----------|------|
| **UDPHeader** | **12 bytes** |
| type (uint16) | 2B |
| sequence_num (uint16) | 2B |
| timestamp (uint64) | 8B |

### Server → Client Messages

| Message | Payload | Total (Header+Payload) |
|---------|---------|------------------------|
| **GameSnapshot (minimal)** | ~12B | **24B** |
| **GameSnapshot (1P, idle)** | 32B | **44B** |
| **GameSnapshot (4P, MAX)** | ~618B | **630B** |
| MissileSpawned | 7B | **19B** |
| MissileDestroyed | 2B | **14B** |
| EnemyDestroyed | 2B | **14B** |
| PlayerJoin | 1B | **13B** |
| PlayerLeave | 1B | **13B** |
| PlayerDamaged | 3B | **15B** |
| PlayerDied | 1B | **13B** |
| BossSpawn | 12B | **24B** |
| BossPhaseChange | 1B | **13B** |
| BossDefeated | 0B | **12B** |
| WeaponChanged | 1B | **13B** |
| WaveCannonFired | 9B | **21B** |
| PowerUpSpawned | 8B | **20B** |
| PowerUpCollected | 4B | **16B** |
| PowerUpExpired | 2B | **14B** |
| ForceStateUpdate | 7B | **19B** |
| HeartBeatAck | 0B | **12B** |

### Client → Server Messages

| Message | Payload | Total |
|---------|---------|-------|
| **PlayerInput** | 4B | **16B** |
| ShootMissile | 0B | **12B** |
| HeartBeat | 0B | **12B** |
| SwitchWeapon | 1B | **13B** |
| ChargeStart | 0B | **12B** |
| ChargeRelease | 1B | **13B** |
| ForceToggle | 0B | **12B** |

### Voice Chat (Separate UDP Port 4126)

| Message | Payload | Total |
|---------|---------|-------|
| VoiceJoin | 38B | **50B** |
| VoiceJoinAck | 1B | **13B** |
| VoiceLeave | 1B | **13B** |
| VoiceFrame | 5-485B | **17-497B** |
| VoiceMute | 2B | **14B** |

---

## 2. GameSnapshot Size Calculation

### Structure Breakdown

```
GameSnapshot =
    1B  (player_count)
  + player_count × 23B  (PlayerState)
  + 1B  (missile_count)
  + missile_count × 8B  (MissileState)
  + 1B  (enemy_count)
  + enemy_count × 8B  (EnemyState)
  + 1B  (enemy_missile_count)
  + enemy_missile_count × 8B  (MissileState)
  + 2B  (wave_number)
  + 1B  (has_boss)
  + [12B if has_boss]  (BossState)
  + 1B  (force_count)
  + force_count × 7B  (ForceStateSnapshot)
  + 1B  (bit_count)
  + bit_count × 6B  (BitDeviceStateSnapshot)
```

### Component Sizes

| Component | WIRE_SIZE | Max Count | Max Total |
|-----------|-----------|-----------|-----------|
| PlayerState | 23B | 4 | 92B |
| MissileState | 8B | 32 | 256B |
| EnemyState | 8B | 16 | 128B |
| Enemy MissileState | 8B | 32 | 256B |
| BossState | 12B | 1 | 12B |
| ForceStateSnapshot | 7B | 4 | 28B |
| BitDeviceStateSnapshot | 6B | 8 | 48B |

### Scenario Examples

| Scenario | Players | Missiles | Enemies | Enemy Missiles | Boss | Forces | Bits | Payload | Total |
|----------|---------|----------|---------|----------------|------|--------|------|---------|-------|
| **Idle (1P)** | 1 | 0 | 0 | 0 | No | 0 | 0 | 32B | **44B** |
| **Early Game (2P)** | 2 | 8 | 4 | 8 | No | 0 | 0 | 180B | **192B** |
| **Mid Game (4P)** | 4 | 20 | 12 | 20 | No | 2 | 4 | 440B | **452B** |
| **Boss Fight (4P MAX)** | 4 | 32 | 16 | 32 | Yes | 4 | 8 | 618B | **630B** |

### Size Formula

```cpp
size_t calculateSnapshotSize(uint8_t players, uint8_t missiles,
                              uint8_t enemies, uint8_t enemyMissiles,
                              bool hasBoss, uint8_t forces, uint8_t bits) {
    return 12  // UDPHeader
         + 1 + players * 23      // PlayerState
         + 1 + missiles * 8      // MissileState
         + 1 + enemies * 8       // EnemyState
         + 1 + enemyMissiles * 8 // Enemy missiles
         + 2                      // wave_number
         + 1 + (hasBoss ? 12 : 0) // BossState
         + 1 + forces * 7        // ForceStateSnapshot
         + 1 + bits * 6;         // BitDeviceStateSnapshot
}
```

---

## 3. Bandwidth Analysis

### Server Broadcast Configuration

| Parameter | Value |
|-----------|-------|
| Tick Rate | 20 Hz |
| Interval | 50 ms |
| Max Players | 4 |

### Per-Client Bandwidth (Server → Client)

| Scenario | Snapshot Size | Snapshots/s | Snapshot BW | Events (est.) | **Total** |
|----------|---------------|-------------|-------------|---------------|-----------|
| Idle | 44B | 20 | 0.88 KB/s | ~0.1 KB/s | **~1 KB/s** |
| Early Game | 192B | 20 | 3.84 KB/s | ~0.5 KB/s | **~4.5 KB/s** |
| Mid Game | 452B | 20 | 9.04 KB/s | ~1.5 KB/s | **~11 KB/s** |
| **Boss (MAX)** | 630B | 20 | 12.6 KB/s | ~3 KB/s | **~16 KB/s** |

### Per-Client Bandwidth (Client → Server)

| Component | Rate | Size | Bandwidth |
|-----------|------|------|-----------|
| PlayerInput | 60 Hz | 16B | 0.96 KB/s |
| ShootMissile | ~10/s max | 12B | ~0.12 KB/s |
| HeartBeat | ~1/s | 12B | ~0.01 KB/s |
| Weapon/Charge | ~2/s | 13B | ~0.03 KB/s |
| **Total** | - | - | **~1.2 KB/s** |

### Server Total Bandwidth

| Direction | Per Player | 2 Players | 4 Players |
|-----------|------------|-----------|-----------|
| S→C (send) | 16 KB/s | 32 KB/s | **64 KB/s** |
| C→S (recv) | 1.2 KB/s | 2.4 KB/s | **4.8 KB/s** |
| **Combined** | 17.2 KB/s | 34.4 KB/s | **~70 KB/s** |

### Voice Chat Bandwidth (Optional)

| Component | Rate | Size | Bandwidth |
|-----------|------|------|-----------|
| VoiceFrame (Opus 32kbps) | 50/s | ~100B avg | ~5 KB/s |
| Relay (4P all talking) | 50×3/s | ~100B | ~15 KB/s recv |

---

## 4. RTT & Frame Timing Analysis

### Network Latency Scenarios

| Condition | RTT | One-Way | Description |
|-----------|-----|---------|-------------|
| **LAN** | <5 ms | <2.5 ms | Local network |
| **Fiber (same region)** | 10-30 ms | 5-15 ms | France ↔ France |
| **Regional** | 30-60 ms | 15-30 ms | France ↔ Germany |
| **Continental** | 60-100 ms | 30-50 ms | France ↔ UK/Spain |
| **Intercontinental** | 100-200 ms | 50-100 ms | France ↔ US East |
| **High Latency** | 200-400 ms | 100-200 ms | France ↔ Asia |

### Frame Budget Analysis

| Client FPS | Frame Time | Snapshots/Frame | Interpolation Window |
|------------|------------|-----------------|----------------------|
| 30 | 33.33 ms | 0.67 | 3 snapshots (150ms) |
| 60 | 16.67 ms | 0.33 | 2-3 snapshots (100-150ms) |
| 120 | 8.33 ms | 0.17 | 2-3 snapshots |
| 144 | 6.94 ms | 0.14 | 2-3 snapshots |

### Input-to-Visual Delay Breakdown

```
Total Delay = Input Processing + Upload + Server Tick + Download + Rendering

Components:
- Input Processing:  ~1 ms (client-side)
- Upload:            RTT/2
- Server Processing: 0-50 ms (avg 25ms at 20Hz)
- Download:          RTT/2
- Rendering:         ~8 ms (at 120fps)

Total = RTT + ~34 ms (fixed overhead)
```

### Delay by RTT

| RTT | Fixed Overhead | **Total Input Lag** | Perceived Quality |
|-----|----------------|---------------------|-------------------|
| 10 ms | 34 ms | **~44 ms** | 🟢 Excellent |
| 30 ms | 34 ms | **~64 ms** | 🟢 Good |
| 60 ms | 34 ms | **~94 ms** | 🟡 Acceptable |
| 100 ms | 34 ms | **~134 ms** | 🟠 Noticeable |
| 150 ms | 34 ms | **~184 ms** | 🟠 Degraded |
| 200 ms | 34 ms | **~234 ms** | 🔴 Poor |
| 300 ms | 34 ms | **~334 ms** | 🔴 Unplayable |

### Jitter Impact

| Jitter | Effect | Mitigation |
|--------|--------|------------|
| <10 ms | Imperceptible | None needed |
| 10-30 ms | Minor stutter | Interpolation buffer |
| 30-50 ms | Visible stutter | Larger buffer (150ms) |
| >50 ms | Significant | Adaptive buffering |

---

## 5. Benchmark Comparisons

### vs Industry Standards

| Game | Genre | Tick Rate | Snapshot Size | Client BW |
|------|-------|-----------|---------------|-----------|
| **R-Type (this)** | Shoot'em up | 20 Hz | 200-630B | 10-16 KB/s |
| Quake III Arena | FPS | 20 Hz | ~200B | ~5 KB/s |
| Team Fortress 2 | FPS | 66 Hz | ~400B | ~30 KB/s |
| Counter-Strike 2 | FPS | 64/128 Hz | ~500B | 50-100 KB/s |
| Overwatch 2 | FPS | 63 Hz | ~1 KB | ~100 KB/s |
| Fortnite | BR | 30 Hz | ~2 KB | ~100 KB/s |
| League of Legends | MOBA | 30 Hz | ~1 KB | ~50 KB/s |
| Rocket League | Sports | 120 Hz | ~300B | ~50 KB/s |

### Protocol Efficiency Analysis

| Metric | R-Type | Best Practice | Rating |
|--------|--------|---------------|--------|
| Header overhead | 12B/packet | 4-8B | 🟡 Standard |
| PlayerState | 23B | 20-30B | 🟢 Optimal |
| MissileState | 8B | 8-12B | 🟢 Optimal |
| EnemyState | 8B | 8-12B | 🟢 Optimal |
| Byte order | Big-endian | Network order | 🟢 Correct |
| Compression | None | Optional | 🟡 Room for improvement |
| Delta compression | No | Optional | 🟡 Room for improvement |

### Potential Optimizations

| Optimization | Current | Optimized | Savings |
|--------------|---------|-----------|---------|
| Varint header | 12B | 6B | 50% header |
| zlib compression | None | ~40% ratio | 40% total |
| Delta snapshots | Full state | Changes only | 60-80% |
| Bit packing | Byte-aligned | Bit-packed | 10-20% |

---

## 6. Worst-Case Stress Test

### Maximum Theoretical Load (4P Boss Fight, Chaos Mode)

#### Event Storm Scenario

| Event Type | Rate | Size | **Bandwidth** |
|------------|------|------|---------------|
| GameSnapshot | 20/s | 630B | 12.60 KB/s |
| MissileSpawned | 40/s | 19B | 0.76 KB/s |
| MissileDestroyed | 40/s | 14B | 0.56 KB/s |
| EnemyDestroyed | 10/s | 14B | 0.14 KB/s |
| PlayerDamaged | 8/s | 15B | 0.12 KB/s |
| PowerUpSpawned | 2/s | 20B | 0.04 KB/s |
| PowerUpCollected | 2/s | 16B | 0.03 KB/s |
| WaveCannonFired | 4/s | 21B | 0.08 KB/s |
| ForceStateUpdate | 4/s | 19B | 0.08 KB/s |
| **S→C Per Client** | - | - | **~15 KB/s** |

#### Server Totals

| Metric | Per Client | 4 Clients |
|--------|------------|-----------|
| S→C Bandwidth | 15 KB/s | **60 KB/s** |
| C→S Bandwidth | 1.2 KB/s | **4.8 KB/s** |
| **Total Bandwidth** | 16.2 KB/s | **~65 KB/s** |

### Packet Rate Analysis

| Direction | Packets/sec (per client) | Total (4P) |
|-----------|--------------------------|------------|
| S→C (snapshots) | 20 pps | 80 pps |
| S→C (events) | ~60 pps | ~240 pps |
| C→S (input) | 60 pps | 240 pps |
| C→S (actions) | ~5 pps | ~20 pps |
| **Combined** | ~145 pps | **~580 pps** |

### Memory Buffer Requirements

| Buffer | Size | Purpose |
|--------|------|---------|
| Send buffer | 4 KB | Outgoing packets |
| Recv buffer | 4 KB | Incoming packets |
| Snapshot history | 3 × 630B = 1.9 KB | Interpolation |
| Input history | 60 × 16B = 960B | Prediction |
| **Total per client** | - | **~11 KB** |

---

## 7. Latency Compensation Recommendations

### Interpolation Settings by RTT

| RTT Range | Buffer Size | Snapshots | Extrapolation |
|-----------|-------------|-----------|---------------|
| <30 ms | 75 ms | 1.5 | None |
| 30-60 ms | 100 ms | 2 | Light |
| 60-100 ms | 150 ms | 3 | Moderate |
| 100-150 ms | 200 ms | 4 | Aggressive |
| >150 ms | 250 ms | 5 | Maximum |

### Client-Side Prediction

```cpp
// Current implementation uses lastAckedInputSeq for reconciliation
struct PlayerState {
    // ...
    uint16_t lastAckedInputSeq;  // Last processed input sequence
    // ...
};
```

| Prediction Type | Without | With | Improvement |
|-----------------|---------|------|-------------|
| Movement | RTT + 25ms | ~0ms | Full RTT |
| Shooting | RTT + 25ms | ~25ms | RTT/2 |
| Collision | Server auth | Server auth | N/A |

### Recommended Client Settings

```cpp
// Adaptive interpolation based on measured RTT
float getInterpolationDelay(float rttMs) {
    if (rttMs < 30)  return 75.0f;
    if (rttMs < 60)  return 100.0f;
    if (rttMs < 100) return 150.0f;
    if (rttMs < 150) return 200.0f;
    return 250.0f;
}

// Jitter buffer for voice chat
float getVoiceJitterBuffer(float jitterMs) {
    return std::max(60.0f, jitterMs * 2.0f);
}
```

### Connection Quality Indicators

| RTT | Jitter | Packet Loss | Quality | Icon |
|-----|--------|-------------|---------|------|
| <50ms | <10ms | <1% | Excellent | 🟢🟢🟢🟢 |
| <80ms | <20ms | <2% | Good | 🟢🟢🟢⚪ |
| <120ms | <30ms | <5% | Fair | 🟢🟢⚪⚪ |
| <200ms | <50ms | <10% | Poor | 🟢⚪⚪⚪ |
| >200ms | >50ms | >10% | Bad | 🔴⚪⚪⚪ |

---

## 8. Summary

### Key Metrics

| Metric | Typical | Maximum | Rating |
|--------|---------|---------|--------|
| **Client Bandwidth (S→C)** | 10 KB/s | 16 KB/s | 🟢 Excellent |
| **Client Bandwidth (C→S)** | 1 KB/s | 1.2 KB/s | 🟢 Excellent |
| **Server Total (4P)** | 50 KB/s | 70 KB/s | 🟢 Very Low |
| **Packet Rate (4P)** | 400 pps | 600 pps | 🟢 Low |
| **Snapshot Size** | 200B | 630B | 🟢 Compact |
| **Tick Rate** | 20 Hz | 20 Hz | 🟡 Standard |
| **Max Playable RTT** | - | ~150 ms | 🟡 Typical |

### Strengths

- ✅ **Very low bandwidth** - Works on 3G/4G mobile connections
- ✅ **Compact binary protocol** - No JSON/XML overhead
- ✅ **Proper byte ordering** - Network byte order (big-endian)
- ✅ **Server-authoritative** - Prevents cheating
- ✅ **Input sequence tracking** - Enables client-side prediction

### Areas for Improvement

- ⚠️ **No delta compression** - Full state sent every tick
- ⚠️ **No packet compression** - Could reduce 30-40% with zlib
- ⚠️ **Fixed tick rate** - Could adapt to network conditions
- ⚠️ **12B header** - Could optimize to 6B with varints

### Compatibility

| Connection Type | Min Speed | Typical Latency | Playable? |
|-----------------|-----------|-----------------|-----------|
| Fiber | 100 Mbps | 10-30 ms | 🟢 Excellent |
| Cable | 50 Mbps | 20-50 ms | 🟢 Excellent |
| DSL | 10 Mbps | 30-60 ms | 🟢 Good |
| 4G LTE | 20 Mbps | 50-100 ms | 🟡 Acceptable |
| 3G | 2 Mbps | 100-300 ms | 🟠 Degraded |
| Satellite | 25 Mbps | 500+ ms | 🔴 Unplayable |

---

## Appendix: Protocol Constants

```cpp
// From Protocol.hpp
static constexpr uint8_t MAX_PLAYERS = 4;
static constexpr uint8_t MAX_MISSILES = 32;
static constexpr uint8_t MAX_ENEMIES = 16;
static constexpr uint8_t MAX_ENEMY_MISSILES = 32;
static constexpr uint8_t MAX_POWERUPS = 8;
static constexpr uint8_t MAX_BITS = 8;

// Sizes
UDPHeader::WIRE_SIZE = 12;
PlayerState::WIRE_SIZE = 23;
MissileState::WIRE_SIZE = 8;
EnemyState::WIRE_SIZE = 8;
BossState::WIRE_SIZE = 12;
ForceStateSnapshot::WIRE_SIZE = 7;
BitDeviceStateSnapshot::WIRE_SIZE = 6;
PlayerInput::WIRE_SIZE = 4;

// Server timing
BROADCAST_RATE = 20 Hz;
BROADCAST_INTERVAL = 50 ms;
```

---

*Document generated from Protocol.hpp analysis*
*Last updated: 2026-01-19*
