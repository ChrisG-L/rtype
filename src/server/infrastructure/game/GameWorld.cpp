/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** GameWorld
*/

#include "infrastructure/game/GameWorld.hpp"
#include "collision/AABB.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>

namespace infrastructure::game {

    GameWorld::GameWorld() : _nextPlayerId(1) {
        std::srand(static_cast<unsigned>(std::time(nullptr)));
    }

    void GameWorld::setGameSpeedPercent(uint16_t percent) {
        std::lock_guard<std::mutex> lock(_mutex);
        _gameSpeedPercent = std::clamp(percent, static_cast<uint16_t>(50), static_cast<uint16_t>(200));
        _gameSpeedMultiplier = static_cast<float>(_gameSpeedPercent) / 100.0f;
    }

    uint8_t GameWorld::findAvailableId() const {
        for (uint8_t id = 1; id <= MAX_PLAYERS; ++id) {
            if (_players.find(id) == _players.end()) {
                return id;
            }
        }
        return 0;
    }

    std::optional<uint8_t> GameWorld::addPlayer(const udp::endpoint& endpoint) {
        std::lock_guard<std::mutex> lock(_mutex);

        for (const auto& [id, player] : _players) {
            if (player.endpoint == endpoint) {
                return id;
            }
        }

        if (_players.size() >= MAX_PLAYERS) {
            return std::nullopt;
        }

        uint8_t newId = findAvailableId();
        if (newId == 0) {
            return std::nullopt;
        }

        uint16_t startX = 100;
        uint16_t startY = static_cast<uint16_t>(100 + (newId - 1) * 100);

        ConnectedPlayer player{
            .id = newId,
            .x = startX,
            .y = startY,
            .health = DEFAULT_HEALTH,
            .alive = true,
            .endpoint = endpoint,
            .lastActivity = std::chrono::steady_clock::now()
        };

        _players[newId] = player;
        return newId;
    }

    void GameWorld::removePlayer(uint8_t playerId) {
        std::lock_guard<std::mutex> lock(_mutex);
        _players.erase(playerId);
        _playerInputs.erase(playerId);        // Clean up inputs
        _playerLastInputSeq.erase(playerId);  // Clean up sequence tracking
    }

    void GameWorld::removePlayerByEndpoint(const udp::endpoint& endpoint) {
        std::lock_guard<std::mutex> lock(_mutex);
        for (auto it = _players.begin(); it != _players.end(); ++it) {
            if (it->second.endpoint == endpoint) {
                _playerInputs.erase(it->first);        // Clean up inputs
                _playerLastInputSeq.erase(it->first);  // Clean up sequence tracking
                _players.erase(it);
                return;
            }
        }
    }

    void GameWorld::applyPlayerInput(uint8_t playerId, uint16_t keys, uint16_t sequenceNum) {
        std::lock_guard<std::mutex> lock(_mutex);
        _playerInputs[playerId] = keys;
        _playerLastInputSeq[playerId] = sequenceNum;
    }

    uint16_t GameWorld::getPlayerLastInputSeq(uint8_t playerId) const {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _playerLastInputSeq.find(playerId);
        return (it != _playerLastInputSeq.end()) ? it->second : 0;
    }

    void GameWorld::updatePlayers(float deltaTime) {
        std::lock_guard<std::mutex> lock(_mutex);

        // Apply game speed multiplier to player movement
        float speed = PLAYER_MOVE_SPEED * _gameSpeedMultiplier * deltaTime;

        for (auto& [id, player] : _players) {
            if (!player.alive) continue;

            // Get input keys for this player (default to 0 if none)
            uint16_t keys = 0;
            auto inputIt = _playerInputs.find(id);
            if (inputIt != _playerInputs.end()) {
                keys = inputIt->second;
            }

            // Calculate movement from inputs
            float newX = static_cast<float>(player.x);
            float newY = static_cast<float>(player.y);

            if (keys & InputKeys::UP)    newY -= speed;
            if (keys & InputKeys::DOWN)  newY += speed;
            if (keys & InputKeys::LEFT)  newX -= speed;
            if (keys & InputKeys::RIGHT) newX += speed;

            // Clamp to screen bounds
            newX = std::clamp(newX, 0.0f, SCREEN_WIDTH - PLAYER_SHIP_WIDTH);
            newY = std::clamp(newY, 0.0f, SCREEN_HEIGHT - PLAYER_SHIP_HEIGHT);

            // Update player position
            player.x = static_cast<uint16_t>(newX);
            player.y = static_cast<uint16_t>(newY);
        }
    }

    std::optional<uint8_t> GameWorld::getPlayerIdByEndpoint(const udp::endpoint& endpoint) {
        std::lock_guard<std::mutex> lock(_mutex);
        for (const auto& [id, player] : _players) {
            if (player.endpoint == endpoint) {
                return id;
            }
        }
        return std::nullopt;
    }

    GameSnapshot GameWorld::getSnapshot() const {
        std::lock_guard<std::mutex> lock(_mutex);
        GameSnapshot snapshot{};
        snapshot.player_count = 0;

        for (const auto& [id, player] : _players) {
            if (snapshot.player_count >= MAX_PLAYERS) break;

            // Get last acked input sequence for this player
            uint16_t lastSeq = 0;
            auto seqIt = _playerLastInputSeq.find(id);
            if (seqIt != _playerLastInputSeq.end()) {
                lastSeq = seqIt->second;
            }

            snapshot.players[snapshot.player_count] = PlayerState{
                .id = player.id,
                .x = player.x,
                .y = player.y,
                .health = player.health,
                .alive = static_cast<uint8_t>(player.alive ? 1 : 0),
                .lastAckedInputSeq = lastSeq
            };
            snapshot.player_count++;
        }

        snapshot.missile_count = 0;
        for (const auto& [id, missile] : _missiles) {
            if (snapshot.missile_count >= MAX_MISSILES) break;
            // Clamp pour éviter UB lors de la conversion float négatif -> uint16_t
            snapshot.missiles[snapshot.missile_count] = MissileState{
                .id = missile.id,
                .owner_id = missile.owner_id,
                .x = static_cast<uint16_t>(std::clamp(missile.x, 0.0f, static_cast<float>(UINT16_MAX))),
                .y = static_cast<uint16_t>(std::clamp(missile.y, 0.0f, static_cast<float>(UINT16_MAX)))
            };
            snapshot.missile_count++;
        }

        snapshot.enemy_count = 0;
        for (const auto& [id, enemy] : _enemies) {
            if (snapshot.enemy_count >= MAX_ENEMIES) break;
            // Clamp pour éviter UB lors de la conversion float négatif -> uint16_t
            snapshot.enemies[snapshot.enemy_count] = EnemyState{
                .id = enemy.id,
                .x = static_cast<uint16_t>(std::clamp(enemy.x, 0.0f, static_cast<float>(UINT16_MAX))),
                .y = static_cast<uint16_t>(std::clamp(enemy.y, 0.0f, static_cast<float>(UINT16_MAX))),
                .health = enemy.health,
                .enemy_type = enemy.enemy_type
            };
            snapshot.enemy_count++;
        }

        snapshot.enemy_missile_count = 0;
        for (const auto& [id, missile] : _enemyMissiles) {
            if (snapshot.enemy_missile_count >= MAX_ENEMY_MISSILES) break;
            // Clamp pour éviter UB lors de la conversion float négatif -> uint16_t
            snapshot.enemy_missiles[snapshot.enemy_missile_count] = MissileState{
                .id = missile.id,
                .owner_id = ENEMY_OWNER_ID,
                .x = static_cast<uint16_t>(std::clamp(missile.x, 0.0f, static_cast<float>(UINT16_MAX))),
                .y = static_cast<uint16_t>(std::clamp(missile.y, 0.0f, static_cast<float>(UINT16_MAX)))
            };
            snapshot.enemy_missile_count++;
        }

        return snapshot;
    }

    uint16_t GameWorld::spawnMissile(uint8_t playerId) {
        std::lock_guard<std::mutex> lock(_mutex);

        auto it = _players.find(playerId);
        if (it == _players.end()) return 0;

        const auto& player = it->second;
        uint16_t missileId = _nextMissileId++;

        Missile missile{
            .id = missileId,
            .owner_id = playerId,
            .x = static_cast<float>(player.x) + MISSILE_SPAWN_OFFSET_X,
            .y = static_cast<float>(player.y) + MISSILE_SPAWN_OFFSET_Y,
            .velocityX = Missile::SPEED
        };

        _missiles[missileId] = missile;
        return missileId;
    }

    void GameWorld::updateMissiles(float deltaTime) {
        std::lock_guard<std::mutex> lock(_mutex);
        _destroyedMissiles.clear();

        // Apply game speed multiplier to missile movement
        float adjustedDelta = deltaTime * _gameSpeedMultiplier;

        for (auto it = _missiles.begin(); it != _missiles.end();) {
            it->second.x += it->second.velocityX * adjustedDelta;

            if (it->second.x > SCREEN_WIDTH) {
                _destroyedMissiles.push_back(it->first);
                it = _missiles.erase(it);
            } else {
                ++it;
            }
        }
    }

    std::vector<uint16_t> GameWorld::getDestroyedMissiles() {
        std::lock_guard<std::mutex> lock(_mutex);
        return _destroyedMissiles;
    }

    std::optional<Missile> GameWorld::getMissile(uint16_t missileId) const {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _missiles.find(missileId);
        if (it == _missiles.end()) return std::nullopt;
        return it->second;
    }

    std::vector<udp::endpoint> GameWorld::getAllEndpoints() const {
        std::lock_guard<std::mutex> lock(_mutex);
        std::vector<udp::endpoint> endpoints;
        endpoints.reserve(_players.size());
        for (const auto& [id, player] : _players) {
            endpoints.push_back(player.endpoint);
        }
        return endpoints;
    }

    size_t GameWorld::getPlayerCount() const {
        std::lock_guard<std::mutex> lock(_mutex);
        return _players.size();
    }

    void GameWorld::updateWaveSpawning(float deltaTime) {
        std::lock_guard<std::mutex> lock(_mutex);

        if (_players.empty()) {
            _waveTimer = 0.0f;
            _spawnQueue.clear();
            return;
        }

        // Apply game speed multiplier to wave timing
        float adjustedDelta = deltaTime * _gameSpeedMultiplier;

        for (auto it = _spawnQueue.begin(); it != _spawnQueue.end();) {
            it->delay -= adjustedDelta;
            if (it->delay <= 0.0f && _enemies.size() < MAX_ENEMIES) {
                uint16_t enemyId = _nextEnemyId++;
                uint8_t typeValue = static_cast<uint8_t>(it->type);
                uint8_t health = Enemy::getHealthForType(it->type);
                float shootInterval = 0.0f;
                switch (it->type) {
                    case EnemyType::Tracker: shootInterval = Enemy::SHOOT_INTERVAL_TRACKER; break;
                    case EnemyType::Zigzag: shootInterval = Enemy::SHOOT_INTERVAL_ZIGZAG; break;
                    case EnemyType::Fast: shootInterval = Enemy::SHOOT_INTERVAL_FAST; break;
                    case EnemyType::Bomber: shootInterval = Enemy::SHOOT_INTERVAL_BOMBER; break;
                    default: shootInterval = Enemy::SHOOT_INTERVAL_BASIC; break;
                }

                Enemy enemy{
                    .id = enemyId,
                    .x = SPAWN_X,
                    .y = it->spawnY,
                    .health = health,
                    .enemy_type = typeValue,
                    .baseY = it->spawnY,
                    .phaseOffset = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * 6.28f,
                    .aliveTime = 0.0f,
                    .shootCooldown = shootInterval * (0.3f + 0.7f * static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)),
                    .targetY = it->spawnY,
                    .zigzagTimer = 0.0f,
                    .zigzagUp = (std::rand() % 2) == 0
                };

                _enemies[enemyId] = enemy;
                it = _spawnQueue.erase(it);
            } else {
                ++it;
            }
        }

        _waveTimer += adjustedDelta;

        if (_waveTimer >= _currentWaveInterval) {
            _waveTimer = 0.0f;
            _waveNumber++;

            _currentWaveInterval = WAVE_INTERVAL_MIN +
                static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * (WAVE_INTERVAL_MAX - WAVE_INTERVAL_MIN);

            uint8_t baseCount = ENEMIES_PER_WAVE_MIN +
                static_cast<uint8_t>(std::rand() % (ENEMIES_PER_WAVE_MAX - ENEMIES_PER_WAVE_MIN + 1));
            uint8_t bonusEnemies = std::min(static_cast<uint8_t>(_waveNumber / 3), static_cast<uint8_t>(2));
            uint8_t enemyCount = std::min(static_cast<uint8_t>(baseCount + bonusEnemies), static_cast<uint8_t>(MAX_ENEMIES - _enemies.size()));

            if (enemyCount == 0) return;

            float cumulativeDelay = 0.0f;
            float ySpacing = (SPAWN_Y_MAX - SPAWN_Y_MIN) / static_cast<float>(enemyCount + 1);

            for (uint8_t i = 0; i < enemyCount; ++i) {
                float spawnDelay = SPAWN_DELAY_MIN +
                    static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * (SPAWN_DELAY_MAX - SPAWN_DELAY_MIN);
                cumulativeDelay += spawnDelay;

                float baseY = SPAWN_Y_MIN + ySpacing * static_cast<float>(i + 1);
                float jitter = (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) - 0.5f) * 100.0f;
                float spawnY = std::clamp(baseY + jitter, SPAWN_Y_MIN, SPAWN_Y_MAX);

                EnemyType type = EnemyType::Basic;
                int roll = std::rand() % 100;

                if (_waveNumber >= 5 && roll < 10) {
                    type = EnemyType::Bomber;
                } else if (_waveNumber >= 4 && roll < 25) {
                    type = EnemyType::Fast;
                } else if (_waveNumber >= 3 && roll < 40) {
                    type = EnemyType::Zigzag;
                } else if (_waveNumber >= 2 && roll < 55) {
                    type = EnemyType::Tracker;
                }

                _spawnQueue.push_back(SpawnEntry{
                    .delay = cumulativeDelay,
                    .spawnY = spawnY,
                    .type = type
                });
            }
        }
    }

    float GameWorld::getNearestPlayerY() const {
        float nearestY = SCREEN_HEIGHT / 2.0f;
        float minDist = SCREEN_WIDTH * 2.0f;

        for (const auto& [id, player] : _players) {
            if (!player.alive) continue;
            float dist = static_cast<float>(player.x);
            if (dist < minDist) {
                minDist = dist;
                nearestY = static_cast<float>(player.y);
            }
        }
        return nearestY;
    }

    void GameWorld::updateEnemyMovement(Enemy& enemy, float deltaTime) {
        EnemyType type = static_cast<EnemyType>(enemy.enemy_type);

        enemy.x += enemy.getSpeedX() * deltaTime;

        switch (type) {
            case EnemyType::Basic: {
                enemy.y = enemy.baseY + Enemy::AMPLITUDE *
                    std::sin(Enemy::FREQUENCY * enemy.aliveTime + enemy.phaseOffset);
                break;
            }

            case EnemyType::Tracker: {
                float targetY = getNearestPlayerY();
                float diff = targetY - enemy.y;
                float maxMove = Enemy::TRACKER_SPEED_Y * deltaTime;

                if (std::abs(diff) > maxMove) {
                    enemy.y += (diff > 0 ? maxMove : -maxMove);
                } else {
                    enemy.y = targetY;
                }
                break;
            }

            case EnemyType::Zigzag: {
                enemy.zigzagTimer += deltaTime;
                if (enemy.zigzagTimer >= Enemy::ZIGZAG_INTERVAL) {
                    enemy.zigzagTimer = 0.0f;
                    enemy.zigzagUp = !enemy.zigzagUp;
                }

                float direction = enemy.zigzagUp ? -1.0f : 1.0f;
                enemy.y += direction * Enemy::ZIGZAG_SPEED_Y * deltaTime;
                break;
            }

            case EnemyType::Fast: {
                float fastAmplitude = Enemy::AMPLITUDE * 0.6f;
                float fastFrequency = Enemy::FREQUENCY * 2.5f;
                enemy.y = enemy.baseY + fastAmplitude *
                    std::sin(fastFrequency * enemy.aliveTime + enemy.phaseOffset);
                break;
            }

            case EnemyType::Bomber: {
                float slowAmplitude = Enemy::AMPLITUDE * 0.3f;
                float slowFrequency = Enemy::FREQUENCY * 0.5f;
                enemy.y = enemy.baseY + slowAmplitude *
                    std::sin(slowFrequency * enemy.aliveTime + enemy.phaseOffset);
                enemy.baseY += 10.0f * deltaTime;
                break;
            }
        }

        enemy.y = std::clamp(enemy.y, 0.0f, SCREEN_HEIGHT - Enemy::HEIGHT);
        enemy.baseY = std::clamp(enemy.baseY, SPAWN_Y_MIN, SPAWN_Y_MAX);
    }

    void GameWorld::updateEnemies(float deltaTime) {
        std::lock_guard<std::mutex> lock(_mutex);
        _destroyedEnemies.clear();

        // Apply game speed multiplier to enemy updates
        float adjustedDelta = deltaTime * _gameSpeedMultiplier;

        for (auto it = _enemies.begin(); it != _enemies.end();) {
            Enemy& enemy = it->second;
            enemy.aliveTime += adjustedDelta;

            updateEnemyMovement(enemy, adjustedDelta);

            enemy.shootCooldown -= adjustedDelta;
            if (enemy.shootCooldown <= 0.0f && enemy.x < SCREEN_WIDTH && enemy.x > 0.0f) {
                enemy.shootCooldown = enemy.getShootInterval();

                EnemyType type = static_cast<EnemyType>(enemy.enemy_type);
                uint8_t missileCount = (type == EnemyType::Bomber) ? 2 : 1;

                for (uint8_t m = 0; m < missileCount && _enemyMissiles.size() < MAX_ENEMY_MISSILES; ++m) {
                    uint16_t missileId = _nextEnemyMissileId++;

                    float missileSpeed = -400.0f;
                    float yOffset = Enemy::HEIGHT / 2.0f;
                    if (type == EnemyType::Fast) {
                        missileSpeed = -550.0f;
                    } else if (type == EnemyType::Bomber) {
                        missileSpeed = -300.0f;
                        yOffset = (m == 0) ? Enemy::HEIGHT * 0.3f : Enemy::HEIGHT * 0.7f;
                    }

                    Missile enemyMissile{
                        .id = missileId,
                        .owner_id = ENEMY_OWNER_ID,
                        .x = enemy.x - 20.0f,
                        .y = enemy.y + yOffset,
                        .velocityX = missileSpeed
                    };
                    _enemyMissiles[missileId] = enemyMissile;
                }
            }

            if (enemy.x < -Enemy::WIDTH) {
                _destroyedEnemies.push_back(it->first);
                it = _enemies.erase(it);
            } else if (enemy.health == 0) {
                _destroyedEnemies.push_back(it->first);
                it = _enemies.erase(it);
            } else {
                ++it;
            }
        }

        // Update enemy missiles with game speed multiplier
        for (auto it = _enemyMissiles.begin(); it != _enemyMissiles.end();) {
            it->second.x += it->second.velocityX * adjustedDelta;

            if (it->second.x < 0.0f) {
                it = _enemyMissiles.erase(it);
            } else {
                ++it;
            }
        }
    }

    void GameWorld::checkCollisions() {
        std::lock_guard<std::mutex> lock(_mutex);
        _playerDamageEvents.clear();
        _deadPlayers.clear();

        for (auto missileIt = _missiles.begin(); missileIt != _missiles.end();) {
            const auto& missile = missileIt->second;
            collision::AABB missileBox(missile.x, missile.y, Missile::WIDTH, Missile::HEIGHT);

            bool missileDestroyed = false;

            for (auto& [enemyId, enemy] : _enemies) {
                collision::AABB enemyBox(enemy.x, enemy.y, Enemy::WIDTH, Enemy::HEIGHT);

                if (missileBox.intersects(enemyBox)) {
                    if (enemy.health > ENEMY_DAMAGE) {
                        enemy.health -= ENEMY_DAMAGE;
                    } else {
                        enemy.health = 0;
                    }

                    _destroyedMissiles.push_back(missileIt->first);
                    missileIt = _missiles.erase(missileIt);
                    missileDestroyed = true;
                    break;
                }
            }

            if (!missileDestroyed) {
                ++missileIt;
            }
        }

        for (auto missileIt = _enemyMissiles.begin(); missileIt != _enemyMissiles.end();) {
            const auto& missile = missileIt->second;
            collision::AABB missileBox(missile.x, missile.y, Missile::WIDTH, Missile::HEIGHT);

            bool missileDestroyed = false;

            for (auto& [playerId, player] : _players) {
                if (!player.alive) continue;

                collision::AABB playerBox(
                    static_cast<float>(player.x),
                    static_cast<float>(player.y),
                    collision::Hitboxes::SHIP_WIDTH,
                    collision::Hitboxes::SHIP_HEIGHT
                );

                if (missileBox.intersects(playerBox)) {
                    bool wasDead = !player.alive;
                    if (player.health > PLAYER_DAMAGE) {
                        player.health -= PLAYER_DAMAGE;
                    } else {
                        player.health = 0;
                        player.alive = false;
                    }

                    _playerDamageEvents.push_back({playerId, PLAYER_DAMAGE});

                    if (!wasDead && !player.alive) {
                        _deadPlayers.push_back(playerId);
                    }

                    missileIt = _enemyMissiles.erase(missileIt);
                    missileDestroyed = true;
                    break;
                }
            }

            if (!missileDestroyed) {
                ++missileIt;
            }
        }
    }

    std::vector<uint16_t> GameWorld::getDestroyedEnemies() {
        std::lock_guard<std::mutex> lock(_mutex);
        return _destroyedEnemies;
    }

    std::vector<std::pair<uint8_t, uint8_t>> GameWorld::getPlayerDamageEvents() {
        std::lock_guard<std::mutex> lock(_mutex);
        return _playerDamageEvents;
    }

    std::vector<uint8_t> GameWorld::getDeadPlayers() {
        std::lock_guard<std::mutex> lock(_mutex);
        return _deadPlayers;
    }

    bool GameWorld::isPlayerAlive(uint8_t playerId) const {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _players.find(playerId);
        if (it == _players.end()) return false;
        return it->second.alive;
    }

    void GameWorld::updatePlayerActivity(uint8_t playerId) {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _players.find(playerId);
        if (it != _players.end()) {
            it->second.lastActivity = std::chrono::steady_clock::now();
        }
    }

    std::vector<uint8_t> GameWorld::checkPlayerTimeouts(std::chrono::milliseconds timeout) {
        std::lock_guard<std::mutex> lock(_mutex);
        std::vector<uint8_t> timedOutPlayers;
        auto now = std::chrono::steady_clock::now();

        for (const auto& [id, player] : _players) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - player.lastActivity
            );
            if (elapsed > timeout) {
                timedOutPlayers.push_back(id);
            }
        }

        for (uint8_t id : timedOutPlayers) {
            _players.erase(id);
        }

        return timedOutPlayers;
    }
}
