# Blob-ECS

A high-performance, lightweight Entity Component System (ECS) library written in modern C++20.

[![License: LGPL v2.1](https://img.shields.io/badge/License-LGPL%20v2.1-blue.svg)](https://www.gnu.org/licenses/lgpl-2.1)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)

## Overview

Blob-ECS is a custom Entity Component System designed for maximum performance and type safety. It achieves **~51.3 million component operations per second** on modern hardware.

### Key Features

#### **High Performance**
- **Sparse Set Architecture**: O(1) component access, insertion, and removal
- **Cache-Friendly Design**: Dense component storage for optimal memory locality
- **Vector-Based Lookup**: Direct array indexing with component type IDs (no hash computation)
- **Zero-Cost Abstractions**: Compile-time optimizations with C++20 concepts
- **Benchmark Results**: ~51M ops/s on Intel i7-12700H (6.2x faster than hash-map based designs)

#### **Type Safety**
- **Compile-Time Type Checking**: C++20 concepts ensure component types are valid
- **Strong Type System**: Components are type-safe throughout execution
- **Template Metaprogramming**: Type errors caught at compile time, not runtime
- **Concept Constraints**: Only default-constructible types can be components

#### **Memory Efficiency**
- **Minimal Entity Overhead**: Entities are simple uint32_t IDs, no heavy objects
- **Packed Component Storage**: Components stored contiguously in memory
- **Efficient Sparse Arrays**: Fast entity-to-component mapping with minimal waste
- **Automatic Memory Management**: RAII-compliant resource handling

#### **Clean Architecture**
- **Separation of Concerns**: Entities, Components, and Systems are decoupled
- **Flexible Querying**: Query entities by component combinations (AllOf, AnyOf, NoneOf)
- **Entity Groups**: Organize entities into logical groups for batch operations
- **System Management**: Tickrate-based system execution with enable/disable support

### Core Design Principles

1. **Entities as Pure IDs**: Entities are lightweight identifiers (uint32_t), reducing overhead
2. **Sparse Set Storage**: Components use sparse sets for O(1) operations with cache efficiency
3. **Type-Indexed Pools**: Each component type has its own pool, accessed via compile-time type IDs
4. **Data-Oriented Design**: Components are stored in packed arrays for better CPU cache utilization
5. **Header-Only Library**: Easy integration, no linking required

### Performance Characteristics

| Operation | Time Complexity | Notes |
|-----------|----------------|-------|
| Component Access | O(1) | Direct sparse set lookup |
| Component Add/Remove | O(1) | Swap-and-pop for removal |
| Entity Query | O(n) | Linear scan with early exit optimization |
| Entity Creation | O(1) | Reuses entity IDs when available |
| Memory Usage | Sparse | Grows with active entities, not max ID |

### Architecture Overview

```
Registry (Component Manager)
├── ComponentPool<Transform> (Sparse Set)
│   ├── Dense: [Transform, Transform, ...]
│   └── Sparse: [index, NULL, index, ...]
├── ComponentPool<Velocity> (Sparse Set)
└── ComponentPool<Health> (Sparse Set)

ECS (Entity Manager + Systems)
├── Entities: [Entity, Entity, ...]
├── Systems: [RenderSystem, PhysicsSystem, ...]
└── Registry (component storage)
```

## Installation
## Installation

### Prerequisites
- C++20 compatible compiler (GCC 10+, Clang 10+, MSVC 2019+)
- git (for cloning)

### Download
With git for auto download:
```bash
git clone https://github.com/guilec06/Blob-ECS.git blob_ecs
```
OR for manual download:
- Download .zip from GitHub
- Extract into `blob_ecs` directory

### Usage
```bash
cd blob_ecs/
cp ecs/*.hpp your/project/directory
```

Or add the `ecs/` directory to your include path:
```bash
g++ -std=c++20 -I/path/to/blob_ecs/ecs your_code.cpp -o your_program
```

*Note: The directory `blob_ecs` can be replaced with anything but ensure consistency in your directory names*

## Quick Start

### Basic Example

```cpp
#include "ECS.hpp"

// Define components
struct Position {
    float x, y;
};

struct Velocity {
    float vx, vy;
};

int main() {
    ECS::ECS ecs;
    
    // Register component types
    ecs.registerComponent<Position>();
    ecs.registerComponent<Velocity>();
    
    // Create an entity
    ECS::EntityID player = ecs.entityCreate();
    
    // Add components
    auto& pos = ecs.entityAddComponent<Position>(player);
    pos.x = 100.0f;
    pos.y = 200.0f;
    
    auto& vel = ecs.entityAddComponent<Velocity>(player);
    vel.vx = 5.0f;
    vel.vy = 0.0f;
    
    // Query entities with specific components
    auto entities = ecs.getEntitiesByComponentsAllOf<Position, Velocity>();
    
    // Access components
    for (auto entity : entities) {
        auto& p = ecs.entityGetComponent<Position>(entity);
        auto& v = ecs.entityGetComponent<Velocity>(entity);
        
        p.x += v.vx;
        p.y += v.vy;
    }
    
    return 0;
}
```

### Using Systems

```cpp
#include "System.hpp"

class MovementSystem : public ECS::ISystem {
public:
    void update(ECS::ECS& ecs, SystemID id, uint32_t msecs) override {
        auto entities = ecs.getEntitiesByComponentsAllOf<Position, Velocity>();
        
        for (auto entity : entities) {
            auto& pos = ecs.entityGetComponent<Position>(entity);
            auto& vel = ecs.entityGetComponent<Velocity>(entity);
            
            pos.x += vel.vx;
            pos.y += vel.vy;
        }
    }
};

int main() {
    ECS::ECS ecs;
    
    ecs.registerComponent<Position>();
    ecs.registerComponent<Velocity>();

    // System tickrate
    int n = 0;
    
    // Register system with tickrate (run once every n + 1 ticks)
    ecs.registerSystem<MovementSystem>(n);
    
    // Game loop
    while (running) {
        ecs.Update(); // Updates all systems based on tickrate
    }
    
    return 0;
}
```

## API Reference

### Entity Management

```cpp
// Create a new entity
EntityID entity = ecs.entityCreate();

// Create entity with group
EntityID entity = ecs.entityCreate(EntityGroup::ENEMIES);

// Destroy entity and all its components
ecs.entityDelete(entity);

// Check if entity is active
bool active = ecs.entityIsActive(entity);
```

### Component Management

```cpp
// Register a component type (required before use)
ecs.registerComponent<MyComponent>();

// Add component to entity (returns reference)
auto& comp = ecs.entityAddComponent<MyComponent>(entity);

// Get component from entity
auto& comp = ecs.entityGetComponent<MyComponent>(entity);

// Remove component from entity
ecs.entityRemoveComponent<MyComponent>(entity);

// Check if entity has component
bool has = ecs.entityHasComponent<MyComponent>(entity);
```

### Entity Queries

```cpp
// Get all entities with ALL specified components
auto entities = ecs.getEntitiesByComponentsAllOf<Transform, Velocity>();

// Get all entities with ANY of the specified components
auto entities = ecs.getEntitiesByComponentsAnyOf<Weapon, Armor>();

// Get all entities in a group (vector of IDs)
auto entities = ecs.getEntityGroup(EntityGroup::ENEMIES);
```

### System Management

```cpp
// Register system with tickrate (ticks between updates)
ecs.registerSystem<PhysicsSystem>(5);

// Disable/enable system
ecs.toggleSystem(system_id);

// Update all active systems (delta_t time since last update)
ecs.Update(dt);
```

## Performance Tips

1. **Pre-register components**: Call `registerComponent()` during initialization, not in hot paths
2. **Use entity queries wisely**: Cache query results if the same entities are processed multiple times
3. **Batch operations**: Process entities in groups rather than individually
4. **Reserve capacity**: If you know entity counts, reserve space in component pools
5. **Avoid frequent add/remove**: Component addition/removal in tight loops can fragment memory

## Benchmarks

Tested on Intel Core i7-12700H:

| Operation | Performance | Configuration |
|-----------|------------|---------------|
| Component Access | 51.3M ops/s | 100k entities, single component |
| Entity Creation | 100k in ~10ms | Bulk creation |
| Component Query | <2ms | 100k entities, 2 components |

## License

This library is licensed under the GNU Lesser General Public License v2.1 (LGPL-2.1).

```
Blob ECS - A lightweight Entity Component System library
Copyright (C) 2025 LECOCQ Guillaume

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.
```

See [LICENSE](LICENSE) for full license text.

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues.

## Author

**Guillaume LECOCQ** - [guilec06](https://github.com/guilec06)

---

*Built with performance and type safety in mind*
