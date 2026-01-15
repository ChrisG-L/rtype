---
tags:
  - technique
  - architecture
---

# Architecture

Vue d'ensemble de l'architecture R-Type.

<div class="grid-cards">
  <div class="card">
    <h3><a href="hexagonale/">🔷 Hexagonale</a></h3>
    <p>Architecture serveur (Ports & Adapters)</p>
  </div>
  <div class="card">
    <h3><a href="client/">🎮 Client</a></h3>
    <p>Scenes, Engine, Plugins</p>
  </div>
  <div class="card">
    <h3><a href="protocoles/">📡 Protocoles</a></h3>
    <p>TCP, UDP, Voice</p>
  </div>
</div>

---

## Vue Globale

```mermaid
flowchart TB
    subgraph Client
        C1[Scenes]
        C2[Engine]
        C3[Graphics]
        C4[Network]
    end

    subgraph Server
        S1[Domain]
        S2[Application]
        S3[Infrastructure]
    end

    C4 <-->|TCP/UDP| S3
    S3 --> S2
    S2 --> S1

    style S1 fill:#7c3aed,color:#fff
    style C1 fill:#f59e0b,color:#000
```

---

## Principes

| Principe | Application |
|----------|-------------|
| **Hexagonal** | Serveur (Ports & Adapters) |
| **Plugin** | Client (backends graphiques via dlopen) |
| **DDD** | Domain-Driven Design serveur |

---

## Technologies

| Composant | Technologie |
|-----------|-------------|
| Langage | C++23 |
| Build | CMake 3.30+, Ninja, vcpkg |
| Réseau | Boost.ASIO |
| Sécurité | OpenSSL (TLS 1.2+) |
| Graphiques | SDL2 / SFML (multi-backend) |
| Audio | PortAudio, Opus |
| BDD | MongoDB |
