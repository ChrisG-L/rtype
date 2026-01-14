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

    GameWorld::GameWorld(boost::asio::io_context& io_ctx)
        : _strand(boost::asio::make_strand(io_ctx))
        , _nextPlayerId(1)
    {
        std::srand(static_cast<unsigned>(std::time(nullptr)));
    }

    void GameWorld::setGameSpeedPercent(uint16_t percent) {
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
        _playerScores[newId] = PlayerScore{};  // Initialize score for new player
        return newId;
    }

    void GameWorld::removePlayer(uint8_t playerId) {
        _players.erase(playerId);
        _playerInputs.erase(playerId);        // Clean up inputs
        _playerLastInputSeq.erase(playerId);  // Clean up sequence tracking
        _playerScores.erase(playerId);        // Clean up scores
    }

    void GameWorld::removePlayerByEndpoint(const udp::endpoint& endpoint) {
        for (auto it = _players.begin(); it != _players.end(); ++it) {
            if (it->second.endpoint == endpoint) {
                _playerInputs.erase(it->first);        // Clean up inputs
                _playerLastInputSeq.erase(it->first);  // Clean up sequence tracking
                _playerScores.erase(it->first);        // Clean up scores
                _players.erase(it);
                return;
            }
        }
    }

    void GameWorld::setPlayerSkin(uint8_t playerId, uint8_t skinId) {
        auto it = _players.find(playerId);
        if (it != _players.end()) {
            // Clamp to valid range (1-6)
            it->second.shipSkin = std::clamp(skinId, static_cast<uint8_t>(1), static_cast<uint8_t>(6));
        }
    }

    void GameWorld::applyPlayerInput(uint8_t playerId, uint16_t keys, uint16_t sequenceNum) {
        // Get previous keys to detect weapon switch "just pressed"
        uint16_t prevKeys = 0;
        auto prevIt = _playerInputs.find(playerId);
        if (prevIt != _playerInputs.end()) {
            prevKeys = prevIt->second;
        }

        // Handle weapon switching (detect rising edge)
        bool weaponNextPressed = (keys & InputKeys::WEAPON_NEXT) && !(prevKeys & InputKeys::WEAPON_NEXT);
        bool weaponPrevPressed = (keys & InputKeys::WEAPON_PREV) && !(prevKeys & InputKeys::WEAPON_PREV);

        if (weaponNextPressed) {
            switchWeapon(playerId, true);
        } else if (weaponPrevPressed) {
            switchWeapon(playerId, false);
        }

        _playerInputs[playerId] = keys;
        _playerLastInputSeq[playerId] = sequenceNum;
    }

    uint16_t GameWorld::getPlayerLastInputSeq(uint8_t playerId) const {
        auto it = _playerLastInputSeq.find(playerId);
        return (it != _playerLastInputSeq.end()) ? it->second : 0;
    }

    void GameWorld::updatePlayers(float deltaTime) {
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
        for (const auto& [id, player] : _players) {
            if (player.endpoint == endpoint) {
                return id;
            }
        }
        return std::nullopt;
    }

    GameSnapshot GameWorld::getSnapshot() const {
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

            // Get score data for this player
            uint32_t score = 0;
            uint16_t kills = 0;
            uint8_t combo = 10;  // 1.0x default
            auto scoreIt = _playerScores.find(id);
            if (scoreIt != _playerScores.end()) {
                score = scoreIt->second.score;
                kills = scoreIt->second.kills;
                combo = scoreIt->second.getComboEncoded();
            }

            snapshot.players[snapshot.player_count] = PlayerState{
                .id = player.id,
                .x = player.x,
                .y = player.y,
                .health = player.health,
                .alive = static_cast<uint8_t>(player.alive ? 1 : 0),
                .lastAckedInputSeq = lastSeq,
                .shipSkin = player.shipSkin,
                .score = score,
                .kills = kills,
                .combo = combo,
                .currentWeapon = static_cast<uint8_t>(player.currentWeapon)
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
                .y = static_cast<uint16_t>(std::clamp(missile.y, 0.0f, static_cast<float>(UINT16_MAX))),
                .weapon_type = static_cast<uint8_t>(missile.weaponType)
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
                .y = static_cast<uint16_t>(std::clamp(missile.y, 0.0f, static_cast<float>(UINT16_MAX))),
                .weapon_type = 0  // Enemy missiles are standard type
            };
            snapshot.enemy_missile_count++;
        }

        // Wave info
        snapshot.wave_number = _waveNumber;

        // Boss state
        if (_boss.has_value() && _boss->isActive) {
            snapshot.has_boss = 1;
            snapshot.boss_state = BossState{
                .id = _boss->id,
                .x = static_cast<uint16_t>(std::clamp(_boss->x, 0.0f, static_cast<float>(UINT16_MAX))),
                .y = static_cast<uint16_t>(std::clamp(_boss->y, 0.0f, static_cast<float>(UINT16_MAX))),
                .max_health = _boss->maxHealth,
                .health = _boss->health,
                .phase = static_cast<uint8_t>(_boss->phase),
                .is_active = 1
            };
        } else {
            snapshot.has_boss = 0;
        }

        return snapshot;
    }

    uint16_t GameWorld::spawnMissile(uint8_t playerId) {
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
        _destroyedMissiles.clear();

        // Apply game speed multiplier to missile movement
        float adjustedDelta = deltaTime * _gameSpeedMultiplier;

        for (auto it = _missiles.begin(); it != _missiles.end();) {
            Missile& missile = it->second;

            // Handle homing missiles
            if (missile.weaponType == WeaponType::Missile && missile.targetEnemyId != 0) {
                float targetX = 0.0f, targetY = 0.0f;
                bool hasTarget = false;

                // Find target position
                if (missile.targetEnemyId == 9999 && _boss.has_value() && _boss->isActive) {
                    // Target is boss
                    targetX = _boss->x + Boss::WIDTH / 2.0f;
                    targetY = _boss->y + Boss::HEIGHT / 2.0f;
                    hasTarget = true;
                } else {
                    auto enemyIt = _enemies.find(missile.targetEnemyId);
                    if (enemyIt != _enemies.end()) {
                        targetX = enemyIt->second.x + Enemy::WIDTH / 2.0f;
                        targetY = enemyIt->second.y + Enemy::HEIGHT / 2.0f;
                        hasTarget = true;
                    }
                }

                if (hasTarget) {
                    // Calculate direction to target
                    float dx = targetX - missile.x;
                    float dy = targetY - missile.y;
                    float dist = std::sqrt(dx * dx + dy * dy);

                    if (dist > 1.0f) {
                        // Normalize and apply speed
                        float speed = Missile::getSpeed(WeaponType::Missile);
                        missile.velocityX = (dx / dist) * speed;
                        missile.velocityY = (dy / dist) * speed;
                    }
                }
            }

            // Update position
            missile.x += missile.velocityX * adjustedDelta;
            missile.y += missile.velocityY * adjustedDelta;

            // Check bounds (remove if off-screen)
            bool outOfBounds = missile.x > SCREEN_WIDTH || missile.x < -50.0f ||
                               missile.y > SCREEN_HEIGHT + 50.0f || missile.y < -50.0f;

            if (outOfBounds) {
                _destroyedMissiles.push_back(it->first);
                it = _missiles.erase(it);
            } else {
                ++it;
            }
        }
    }

    std::vector<uint16_t> GameWorld::getDestroyedMissiles() {
        return _destroyedMissiles;
    }

    std::optional<Missile> GameWorld::getMissile(uint16_t missileId) const {
        auto it = _missiles.find(missileId);
        if (it == _missiles.end()) return std::nullopt;
        return it->second;
    }

    std::vector<udp::endpoint> GameWorld::getAllEndpoints() const {
        std::vector<udp::endpoint> endpoints;
        endpoints.reserve(_players.size());
        for (const auto& [id, player] : _players) {
            endpoints.push_back(player.endpoint);
        }
        return endpoints;
    }

    size_t GameWorld::getPlayerCount() const {
        return _players.size();
    }

    void GameWorld::updateWaveSpawning(float deltaTime) {
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

        // Stop spawning new waves once we've reached the boss wave (wait for boss fight)
        if (_waveNumber >= BOSS_SPAWN_WAVE && !_boss.has_value()) {
            // Don't spawn new waves - waiting for boss to spawn (when enemies are cleared)
            return;
        }

        // Also don't spawn new waves during boss fight
        if (_boss.has_value() && _boss->isActive) {
            return;
        }

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
            it->second.y += it->second.velocityY * adjustedDelta;

            // Remove if out of bounds (left, top, or bottom)
            if (it->second.x < 0.0f || it->second.y < 0.0f || it->second.y > SCREEN_HEIGHT) {
                it = _enemyMissiles.erase(it);
            } else {
                ++it;
            }
        }
    }

    void GameWorld::checkCollisions() {
        _playerDamageEvents.clear();
        _deadPlayers.clear();

        for (auto missileIt = _missiles.begin(); missileIt != _missiles.end();) {
            const auto& missile = missileIt->second;
            collision::AABB missileBox(missile.x, missile.y, Missile::WIDTH, Missile::HEIGHT);

            bool missileDestroyed = false;

            for (auto& [enemyId, enemy] : _enemies) {
                collision::AABB enemyBox(enemy.x, enemy.y, Enemy::WIDTH, Enemy::HEIGHT);

                if (missileBox.intersects(enemyBox)) {
                    bool wasAlive = enemy.health > 0;
                    // Use weapon-specific damage
                    uint8_t damage = Missile::getDamage(missile.weaponType);
                    if (enemy.health > damage) {
                        enemy.health -= damage;
                    } else {
                        enemy.health = 0;
                    }

                    // Award score if enemy was killed
                    if (wasAlive && enemy.health == 0) {
                        awardKillScore(missile.owner_id, static_cast<EnemyType>(enemy.enemy_type));
                    }

                    _destroyedMissiles.push_back(missileIt->first);
                    missileIt = _missiles.erase(missileIt);
                    missileDestroyed = true;
                    break;
                }
            }

            // Check boss collision if not already destroyed
            if (!missileDestroyed && _boss.has_value() && _boss->isActive) {
                collision::AABB bossBox(_boss->x, _boss->y, Boss::WIDTH, Boss::HEIGHT);
                if (missileBox.intersects(bossBox)) {
                    // Use weapon-specific damage for boss
                    uint8_t damage = Missile::getDamage(missile.weaponType);
                    damageBoss(damage, missile.owner_id);

                    _destroyedMissiles.push_back(missileIt->first);
                    missileIt = _missiles.erase(missileIt);
                    missileDestroyed = true;
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

                    // Reset combo on damage
                    onPlayerDamaged(playerId);

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
        return _destroyedEnemies;
    }

    std::vector<std::pair<uint8_t, uint8_t>> GameWorld::getPlayerDamageEvents() {
        return _playerDamageEvents;
    }

    std::vector<uint8_t> GameWorld::getDeadPlayers() {
        return _deadPlayers;
    }

    bool GameWorld::isPlayerAlive(uint8_t playerId) const {
        auto it = _players.find(playerId);
        if (it == _players.end()) return false;
        return it->second.alive;
    }

    void GameWorld::updatePlayerActivity(uint8_t playerId) {
        auto it = _players.find(playerId);
        if (it != _players.end()) {
            it->second.lastActivity = std::chrono::steady_clock::now();
        }
    }

    std::vector<uint8_t> GameWorld::checkPlayerTimeouts(std::chrono::milliseconds timeout) {
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

    // ═══════════════════════════════════════════════════════════════════
    // Score System (Gameplay Phase 2)
    // ═══════════════════════════════════════════════════════════════════

    uint16_t GameWorld::getEnemyPointValue(EnemyType type) const {
        switch (type) {
            case EnemyType::Basic:   return POINTS_BASIC;
            case EnemyType::Tracker: return POINTS_TRACKER;
            case EnemyType::Zigzag:  return POINTS_ZIGZAG;
            case EnemyType::Fast:    return POINTS_FAST;
            case EnemyType::Bomber:  return POINTS_BOMBER;
            default: return POINTS_BASIC;
        }
    }

    void GameWorld::awardKillScore(uint8_t playerId, EnemyType enemyType) {
        auto it = _playerScores.find(playerId);
        if (it == _playerScores.end()) {
            _playerScores[playerId] = PlayerScore{};
            it = _playerScores.find(playerId);
        }

        PlayerScore& score = it->second;

        uint16_t basePoints = getEnemyPointValue(enemyType);
        uint32_t points = static_cast<uint32_t>(basePoints * score.comboMultiplier);

        score.score += points;
        score.kills++;

        // Increase combo (max 3.0x)
        score.comboMultiplier = std::min(COMBO_MAX, score.comboMultiplier + COMBO_INCREMENT);
        score.comboTimer = 0.0f;  // Reset combo timer
    }

    void GameWorld::updateComboTimers(float deltaTime) {
        for (auto& [playerId, score] : _playerScores) {
            score.comboTimer += deltaTime;
            if (score.comboTimer > COMBO_DECAY_TIME) {
                score.comboMultiplier = 1.0f;  // Reset combo
            }
        }
    }

    void GameWorld::onPlayerDamaged(uint8_t playerId) {
        auto it = _playerScores.find(playerId);
        if (it != _playerScores.end()) {
            it->second.tookDamageThisWave = true;
            // Reset combo on damage
            it->second.comboMultiplier = 1.0f;
        }
    }

    const PlayerScore& GameWorld::getPlayerScore(uint8_t playerId) const {
        static const PlayerScore emptyScore{};
        auto it = _playerScores.find(playerId);
        if (it == _playerScores.end()) return emptyScore;
        return it->second;
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // Boss System (Gameplay Phase 2) - EPIC VERSION
    // ═══════════════════════════════════════════════════════════════════════════

    void GameWorld::checkBossSpawn() {
        // Don't spawn if boss already exists or not enough waves
        if (_boss.has_value() || _waveNumber < BOSS_SPAWN_WAVE) return;

        // Only spawn when all enemies are cleared
        if (!_enemies.empty()) return;

        // EPIC BOSS ENTRANCE!
        Boss boss;
        boss.id = 9999;
        boss.x = SCREEN_WIDTH + 100.0f;  // Start off-screen for dramatic entrance
        boss.y = SCREEN_HEIGHT / 2.0f;
        boss.baseX = SCREEN_WIDTH - 200.0f;  // Target X position
        boss.baseY = SCREEN_HEIGHT / 2.0f;
        boss.velocityX = -300.0f;  // Entrance speed
        boss.velocityY = 0.0f;
        boss.maxHealth = BOSS_MAX_HEALTH;
        boss.health = BOSS_MAX_HEALTH;
        boss.phase = BossPhase::Phase1;
        boss.currentAttack = BossAttack::Idle;
        boss.movement = BossMovement::Sinusoidal;
        boss.isActive = true;

        // Reset all timers
        boss.phaseTimer = 0.0f;
        boss.attackTimer = 0.0f;
        boss.moveTimer = 0.0f;
        boss.attackCooldown = 2.0f;  // Initial delay before first attack
        boss.stateTimer = 0.0f;

        // Reset attack state
        boss.isCharging = false;
        boss.isDashing = false;
        boss.chargeProgress = 0.0f;
        boss.laserWarning = false;
        boss.minionsSpawned = 0;
        boss.minionCooldown = 0.0f;
        boss.isEnraged = false;
        boss.enrageTimer = 0.0f;
        boss.attackPatternIndex = 0;

        _boss = boss;
    }

    void GameWorld::updateBoss(float deltaTime) {
        if (!_boss.has_value() || !_boss->isActive) return;

        Boss& boss = _boss.value();
        float adjustedDelta = deltaTime * _gameSpeedMultiplier;

        // Update timers
        boss.phaseTimer += adjustedDelta;
        boss.moveTimer += adjustedDelta;

        // Check phase transitions based on HP percentage
        float hpRatio = static_cast<float>(boss.health) / static_cast<float>(boss.maxHealth);

        if (hpRatio <= BOSS_PHASE3_THRESHOLD && boss.phase != BossPhase::Phase3) {
            boss.phase = BossPhase::Phase3;
            boss.isEnraged = true;
            boss.enrageTimer = 0.0f;
            boss.attackCooldown = 0.5f;  // Immediate attack when enraged
            boss.minionsSpawned = 0;     // Reset minion count for phase 3
        } else if (hpRatio <= BOSS_PHASE2_THRESHOLD && boss.phase == BossPhase::Phase1) {
            boss.phase = BossPhase::Phase2;
            boss.minionsSpawned = 0;
            boss.attackCooldown = 0.5f;
        }

        // Update movement
        updateBossMovement(adjustedDelta);

        // Update attacks
        updateBossAttacks(adjustedDelta);

        // Minion spawning (Phase 2+)
        if (boss.phase >= BossPhase::Phase2) {
            boss.minionCooldown -= adjustedDelta;
            uint8_t maxMinions = (boss.phase == BossPhase::Phase3) ? Boss::MAX_MINIONS_P3 : Boss::MAX_MINIONS_P2;

            if (boss.minionCooldown <= 0.0f && boss.minionsSpawned < maxMinions) {
                bossSpawnMinions();
                boss.minionCooldown = Boss::MINION_SPAWN_INTERVAL;
            }
        }

        // Enrage timer effects
        if (boss.isEnraged) {
            boss.enrageTimer += adjustedDelta;
        }
    }

    void GameWorld::updateBossMovement(float deltaTime) {
        Boss& boss = _boss.value();

        // During dash, only update dash movement
        if (boss.isDashing) {
            bossUpdateDash(deltaTime);
            return;
        }

        // Entrance animation (moving to base position)
        if (boss.x > boss.baseX) {
            boss.x += boss.velocityX * deltaTime;
            if (boss.x <= boss.baseX) {
                boss.x = boss.baseX;
                boss.velocityX = 0.0f;
            }
            return;
        }

        // Normal movement patterns
        switch (boss.movement) {
            case BossMovement::Sinusoidal: {
                // Classic sine wave vertical movement
                float amplitude = boss.isEnraged ? 150.0f : 100.0f;
                float speed = boss.isEnraged ? 1.0f : 0.5f;
                boss.y = boss.baseY + std::sin(boss.moveTimer * speed) * amplitude;
                break;
            }

            case BossMovement::Figure8: {
                // Figure-8 pattern
                float t = boss.moveTimer * 0.4f;
                boss.x = boss.baseX + std::sin(t) * 80.0f;
                boss.y = boss.baseY + std::sin(t * 2.0f) * 120.0f;
                break;
            }

            case BossMovement::Aggressive: {
                // Chase nearest player
                float targetY = getNearestPlayerY();
                float dy = targetY - boss.y;
                float moveSpeed = Boss::MOVE_SPEED * (boss.isEnraged ? 1.5f : 1.0f);

                if (std::abs(dy) > 10.0f) {
                    boss.y += (dy > 0 ? 1.0f : -1.0f) * moveSpeed * deltaTime;
                }

                // Also move forward when aggressive
                if (boss.x > 1400.0f) {
                    boss.x -= 50.0f * deltaTime;
                }
                break;
            }

            case BossMovement::Retreat: {
                // Back away towards right side
                if (boss.x < SCREEN_WIDTH - 180.0f) {
                    boss.x += Boss::MOVE_SPEED * 0.5f * deltaTime;
                }
                // Continue sinusoidal Y
                boss.y = boss.baseY + std::sin(boss.moveTimer * 0.8f) * 80.0f;
                break;
            }

            default:
                break;
        }

        // Clamp position
        boss.y = std::clamp(boss.y, 50.0f, SCREEN_HEIGHT - Boss::HEIGHT - 50.0f);
        boss.x = std::clamp(boss.x, SCREEN_WIDTH * 0.4f, SCREEN_WIDTH - 100.0f);
    }

    void GameWorld::updateBossAttacks(float deltaTime) {
        Boss& boss = _boss.value();

        // Update cooldown
        if (boss.attackCooldown > 0.0f) {
            boss.attackCooldown -= deltaTime;
            return;
        }

        // If currently executing an attack
        if (boss.currentAttack != BossAttack::Idle) {
            boss.stateTimer += deltaTime;

            // Handle laser attack separately (needs continuous update)
            if (boss.currentAttack == BossAttack::LaserCharge) {
                bossAttackLaser(deltaTime);
                return;
            }

            // Attack finished
            boss.currentAttack = BossAttack::Idle;
            boss.stateTimer = 0.0f;
        }

        // Select and execute next attack
        selectNextBossAttack();
    }

    void GameWorld::selectNextBossAttack() {
        Boss& boss = _boss.value();

        // Attack patterns per phase
        static const std::vector<BossAttack> phase1Pattern = {
            BossAttack::LinearShots,
            BossAttack::SpreadShot,
            BossAttack::LinearShots,
            BossAttack::CircularBarrage
        };

        static const std::vector<BossAttack> phase2Pattern = {
            BossAttack::LinearShots,
            BossAttack::HomingSwarm,
            BossAttack::SpreadShot,
            BossAttack::LaserCharge,
            BossAttack::CircularBarrage,
            BossAttack::SpawnMinions
        };

        static const std::vector<BossAttack> phase3Pattern = {
            BossAttack::DashCharge,
            BossAttack::CircularBarrage,
            BossAttack::HomingSwarm,
            BossAttack::Teleport,
            BossAttack::LinearShots,
            BossAttack::SpreadShot,
            BossAttack::LaserCharge,
            BossAttack::CircularBarrage,
            BossAttack::SpawnMinions
        };

        const std::vector<BossAttack>* pattern;
        float cooldown;

        switch (boss.phase) {
            case BossPhase::Phase1:
                pattern = &phase1Pattern;
                cooldown = Boss::ATTACK_INTERVAL_P1;
                break;
            case BossPhase::Phase2:
                pattern = &phase2Pattern;
                cooldown = Boss::ATTACK_INTERVAL_P2;
                break;
            case BossPhase::Phase3:
            default:
                pattern = &phase3Pattern;
                cooldown = Boss::ATTACK_INTERVAL_P3;
                // Enraged: even faster attacks
                if (boss.isEnraged && boss.enrageTimer > 5.0f) {
                    cooldown *= 0.7f;
                }
                break;
        }

        // Get next attack from pattern
        BossAttack nextAttack = (*pattern)[boss.attackPatternIndex % pattern->size()];
        boss.attackPatternIndex++;

        // Execute the attack
        executeBossAttack(nextAttack);
        boss.attackCooldown = cooldown;
    }

    void GameWorld::executeBossAttack(BossAttack attack) {
        Boss& boss = _boss.value();
        boss.currentAttack = attack;
        boss.stateTimer = 0.0f;

        switch (attack) {
            case BossAttack::LinearShots:
                bossAttackLinearShots();
                break;
            case BossAttack::SpreadShot:
                bossAttackSpreadShot();
                break;
            case BossAttack::CircularBarrage:
                bossAttackCircularBarrage();
                break;
            case BossAttack::HomingSwarm:
                bossAttackHomingSwarm();
                break;
            case BossAttack::LaserCharge:
                boss.laserWarning = true;
                boss.isCharging = true;
                boss.chargeProgress = 0.0f;
                // Movement stops during laser
                boss.movement = BossMovement::Idle;
                break;
            case BossAttack::DashCharge:
                bossStartDash();
                break;
            case BossAttack::SpawnMinions:
                bossSpawnMinions();
                boss.currentAttack = BossAttack::Idle;  // Instant
                break;
            case BossAttack::Teleport:
                bossTeleport();
                boss.currentAttack = BossAttack::Idle;  // Instant
                break;
            default:
                boss.currentAttack = BossAttack::Idle;
                break;
        }
    }

    // ─────────────────────────────────────────────────────────────────────────────
    // Individual Attack Implementations
    // ─────────────────────────────────────────────────────────────────────────────

    void GameWorld::bossAttackLinearShots() {
        Boss& boss = _boss.value();
        int shotCount = boss.isEnraged ? 5 : 3;

        for (int i = -(shotCount/2); i <= shotCount/2; ++i) {
            if (_enemyMissiles.size() >= MAX_ENEMY_MISSILES) break;

            uint16_t missileId = _nextEnemyMissileId++;
            float speed = boss.isEnraged ? -500.0f : -400.0f;
            Missile missile{
                .id = missileId,
                .owner_id = ENEMY_OWNER_ID,
                .x = boss.x - 50.0f,
                .y = boss.y + Boss::HEIGHT / 2.0f + i * 35.0f,
                .velocityX = speed,
                .velocityY = 0.0f
            };
            _enemyMissiles[missileId] = missile;
        }
    }

    void GameWorld::bossAttackSpreadShot() {
        Boss& boss = _boss.value();
        int shotCount = boss.isEnraged ? 9 : 7;
        float spreadAngle = boss.isEnraged ? 120.0f : 90.0f;  // degrees
        float startAngle = 180.0f - spreadAngle / 2.0f;  // Facing left

        for (int i = 0; i < shotCount; ++i) {
            if (_enemyMissiles.size() >= MAX_ENEMY_MISSILES) break;

            float angle = startAngle + (spreadAngle / (shotCount - 1)) * i;
            float rad = angle * 3.14159f / 180.0f;
            float speed = 350.0f;

            uint16_t missileId = _nextEnemyMissileId++;
            Missile missile{
                .id = missileId,
                .owner_id = ENEMY_OWNER_ID,
                .x = boss.x,
                .y = boss.y + Boss::HEIGHT / 2.0f,
                .velocityX = std::cos(rad) * speed,
                .velocityY = std::sin(rad) * speed
            };
            _enemyMissiles[missileId] = missile;
        }
    }

    void GameWorld::bossAttackCircularBarrage() {
        Boss& boss = _boss.value();
        int missileCount = boss.isEnraged ? 16 : 12;
        float speed = boss.isEnraged ? 300.0f : 250.0f;

        // Rotating offset for variety
        float offsetAngle = boss.moveTimer * 0.5f;

        for (int i = 0; i < missileCount; ++i) {
            if (_enemyMissiles.size() >= MAX_ENEMY_MISSILES) break;

            float angle = offsetAngle + i * (2.0f * 3.14159f / missileCount);
            float vx = std::cos(angle) * speed;
            float vy = std::sin(angle) * speed;

            uint16_t missileId = _nextEnemyMissileId++;
            Missile missile{
                .id = missileId,
                .owner_id = ENEMY_OWNER_ID,
                .x = boss.x + Boss::WIDTH / 2.0f,
                .y = boss.y + Boss::HEIGHT / 2.0f,
                .velocityX = vx - 50.0f,  // Slight leftward bias
                .velocityY = vy
            };
            _enemyMissiles[missileId] = missile;
        }
    }

    void GameWorld::bossAttackHomingSwarm() {
        Boss& boss = _boss.value();
        int missileCount = boss.isEnraged ? 6 : 4;

        // Find player positions
        std::vector<std::pair<float, float>> playerPositions;
        for (const auto& [id, player] : _players) {
            if (player.alive) {
                playerPositions.emplace_back(player.x, player.y);
            }
        }

        if (playerPositions.empty()) return;

        for (int i = 0; i < missileCount; ++i) {
            if (_enemyMissiles.size() >= MAX_ENEMY_MISSILES) break;

            // Target a random alive player
            const auto& target = playerPositions[i % playerPositions.size()];
            float dx = target.first - boss.x;
            float dy = target.second - (boss.y + Boss::HEIGHT / 2.0f);
            float dist = std::sqrt(dx * dx + dy * dy);

            if (dist < 1.0f) dist = 1.0f;

            float speed = 280.0f;
            float offsetY = (i - missileCount/2) * 20.0f;  // Spread out vertically

            uint16_t missileId = _nextEnemyMissileId++;
            Missile missile{
                .id = missileId,
                .owner_id = ENEMY_OWNER_ID,
                .x = boss.x - 30.0f,
                .y = boss.y + Boss::HEIGHT / 2.0f + offsetY,
                .velocityX = (dx / dist) * speed,
                .velocityY = (dy / dist) * speed
            };
            _enemyMissiles[missileId] = missile;
        }
    }

    void GameWorld::bossAttackLaser(float deltaTime) {
        Boss& boss = _boss.value();

        if (boss.isCharging) {
            boss.chargeProgress += deltaTime / Boss::CHARGE_TIME;

            if (boss.chargeProgress >= 1.0f) {
                boss.isCharging = false;
                boss.laserWarning = false;
                boss.stateTimer = 0.0f;

                // Fire laser beam! (rapid stream of projectiles)
                float targetY = getNearestPlayerY();
                boss.laserAngle = std::atan2(targetY - (boss.y + Boss::HEIGHT/2.0f), -500.0f);
            }
        } else {
            // Laser is firing
            boss.stateTimer += deltaTime;

            // Spawn rapid projectiles along laser path
            static float laserFireTimer = 0.0f;
            laserFireTimer += deltaTime;

            if (laserFireTimer >= 0.05f && boss.stateTimer < Boss::LASER_DURATION) {
                laserFireTimer = 0.0f;

                if (_enemyMissiles.size() < MAX_ENEMY_MISSILES) {
                    float speed = 800.0f;
                    uint16_t missileId = _nextEnemyMissileId++;
                    Missile missile{
                        .id = missileId,
                        .owner_id = ENEMY_OWNER_ID,
                        .x = boss.x - 20.0f,
                        .y = boss.y + Boss::HEIGHT / 2.0f,
                        .velocityX = std::cos(boss.laserAngle) * speed,
                        .velocityY = std::sin(boss.laserAngle) * speed
                    };
                    _enemyMissiles[missileId] = missile;
                }
            }

            // End laser attack
            if (boss.stateTimer >= Boss::LASER_DURATION + 0.5f) {
                boss.currentAttack = BossAttack::Idle;
                boss.movement = BossMovement::Sinusoidal;  // Resume normal movement
            }
        }
    }

    void GameWorld::bossStartDash() {
        Boss& boss = _boss.value();

        // Find nearest player for dash target
        float targetX = 200.0f;  // Default: dash to left side
        float targetY = boss.y;

        float nearestDist = 99999.0f;
        for (const auto& [id, player] : _players) {
            if (!player.alive) continue;
            float dx = player.x - boss.x;
            float dy = player.y - boss.y;
            float dist = std::sqrt(dx * dx + dy * dy);
            if (dist < nearestDist) {
                nearestDist = dist;
                targetX = player.x + 100.0f;  // Dash past the player
                targetY = player.y;
            }
        }

        boss.isDashing = true;
        boss.dashTargetX = std::clamp(targetX, 100.0f, SCREEN_WIDTH - 200.0f);
        boss.dashTargetY = std::clamp(targetY, 100.0f, SCREEN_HEIGHT - 200.0f);

        // Calculate velocity
        float dx = boss.dashTargetX - boss.x;
        float dy = boss.dashTargetY - boss.y;
        float dist = std::sqrt(dx * dx + dy * dy);
        if (dist > 1.0f) {
            boss.velocityX = (dx / dist) * Boss::DASH_SPEED;
            boss.velocityY = (dy / dist) * Boss::DASH_SPEED;
        }
    }

    void GameWorld::bossUpdateDash(float deltaTime) {
        Boss& boss = _boss.value();

        boss.x += boss.velocityX * deltaTime;
        boss.y += boss.velocityY * deltaTime;

        // Check if reached target (or close enough)
        float dx = boss.dashTargetX - boss.x;
        float dy = boss.dashTargetY - boss.y;
        float dist = std::sqrt(dx * dx + dy * dy);

        if (dist < 30.0f || boss.x < 100.0f) {
            boss.isDashing = false;
            boss.velocityX = 0.0f;
            boss.velocityY = 0.0f;

            // Return to base position
            boss.baseX = SCREEN_WIDTH - 200.0f;
            boss.x = std::max(boss.x, boss.baseX - 100.0f);

            boss.currentAttack = BossAttack::Idle;
        }

        // Clamp during dash
        boss.x = std::clamp(boss.x, 50.0f, SCREEN_WIDTH - 50.0f);
        boss.y = std::clamp(boss.y, 50.0f, SCREEN_HEIGHT - Boss::HEIGHT - 50.0f);
    }

    void GameWorld::bossSpawnMinions() {
        Boss& boss = _boss.value();
        uint8_t maxMinions = (boss.phase == BossPhase::Phase3) ? Boss::MAX_MINIONS_P3 : Boss::MAX_MINIONS_P2;

        if (boss.minionsSpawned >= maxMinions) return;
        if (_enemies.size() >= MAX_ENEMIES) return;

        // Spawn 2-3 minions near the boss
        int spawnCount = (boss.phase == BossPhase::Phase3) ? 3 : 2;

        for (int i = 0; i < spawnCount && boss.minionsSpawned < maxMinions; ++i) {
            if (_enemies.size() >= MAX_ENEMIES) break;

            uint16_t enemyId = _nextEnemyId++;
            float spawnY = boss.y + (i - 1) * 80.0f;
            spawnY = std::clamp(spawnY, SPAWN_Y_MIN, SPAWN_Y_MAX);

            // Spawn Fast or Tracker type minions
            EnemyType type = (i % 2 == 0) ? EnemyType::Fast : EnemyType::Tracker;
            uint8_t health = Enemy::getHealthForType(type);

            Enemy enemy{
                .id = enemyId,
                .x = boss.x - 50.0f,
                .y = spawnY,
                .health = health,
                .enemy_type = static_cast<uint8_t>(type),
                .baseY = spawnY,
                .phaseOffset = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * 6.28f,
                .aliveTime = 0.0f,
                .shootCooldown = Enemy::SHOOT_INTERVAL_FAST,
                .targetY = spawnY,
                .zigzagTimer = 0.0f,
                .zigzagUp = (i % 2 == 0)
            };

            _enemies[enemyId] = enemy;
            boss.minionsSpawned++;
        }
    }

    void GameWorld::bossTeleport() {
        Boss& boss = _boss.value();

        // Teleport to a random position on the right side
        float newX = SCREEN_WIDTH - 150.0f - static_cast<float>(std::rand() % 200);
        float newY = 150.0f + static_cast<float>(std::rand() % static_cast<int>(SCREEN_HEIGHT - 350.0f));

        boss.x = newX;
        boss.y = newY;
        boss.baseX = newX;
        boss.baseY = newY;

        // Briefly change movement pattern
        boss.movement = BossMovement::Aggressive;

        // Spawn circular barrage on teleport for effect
        bossAttackCircularBarrage();
    }

    void GameWorld::damageBoss(uint8_t damage, uint8_t playerId) {
        if (!_boss.has_value() || !_boss->isActive) return;

        Boss& boss = _boss.value();

        if (boss.health > damage) {
            boss.health -= damage;

            // Rage trigger at certain HP thresholds
            float hpRatio = static_cast<float>(boss.health) / static_cast<float>(boss.maxHealth);
            if (hpRatio < 0.20f && !boss.isEnraged) {
                // MAXIMUM RAGE at 20% HP
                boss.isEnraged = true;
                boss.movement = BossMovement::Aggressive;
            }
        } else {
            boss.health = 0;
            boss.isActive = false;

            // EPIC BOSS DEFEATED!
            auto it = _playerScores.find(playerId);
            if (it != _playerScores.end()) {
                // Bonus multiplier based on phase (more points for higher phases)
                uint32_t phaseMultiplier = static_cast<uint32_t>(boss.phase) + 1;
                it->second.score += POINTS_BOSS * phaseMultiplier;
                it->second.kills++;
                // Big combo boost
                it->second.comboMultiplier = COMBO_MAX;
                it->second.comboTimer = 0.0f;
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // Weapon System (Gameplay Phase 2)
    // ═══════════════════════════════════════════════════════════════════════════

    void GameWorld::switchWeapon(uint8_t playerId, bool next) {
        auto it = _players.find(playerId);
        if (it == _players.end()) return;

        uint8_t current = static_cast<uint8_t>(it->second.currentWeapon);
        uint8_t count = MAX_WEAPON_TYPES;

        if (next) {
            current = (current + 1) % count;
        } else {
            current = (current + count - 1) % count;
        }

        it->second.currentWeapon = static_cast<WeaponType>(current);
    }

    WeaponType GameWorld::getPlayerWeapon(uint8_t playerId) const {
        auto it = _players.find(playerId);
        if (it != _players.end()) {
            return it->second.currentWeapon;
        }
        return WeaponType::Standard;
    }

    void GameWorld::updateShootCooldowns(float deltaTime) {
        float adjustedDelta = deltaTime * _gameSpeedMultiplier;
        for (auto& [id, player] : _players) {
            if (player.shootCooldown > 0.0f) {
                player.shootCooldown -= adjustedDelta;
                if (player.shootCooldown < 0.0f) {
                    player.shootCooldown = 0.0f;
                }
            }
        }
    }

    bool GameWorld::canPlayerShoot(uint8_t playerId) const {
        auto it = _players.find(playerId);
        if (it == _players.end() || !it->second.alive) {
            return false;
        }
        return it->second.shootCooldown <= 0.0f;
    }

    std::vector<uint16_t> GameWorld::spawnMissileWithWeapon(uint8_t playerId) {
        std::vector<uint16_t> spawnedIds;

        auto it = _players.find(playerId);
        if (it == _players.end() || !it->second.alive) {
            return spawnedIds;
        }

        // Check cooldown
        if (!canPlayerShoot(playerId)) {
            return spawnedIds;
        }

        ConnectedPlayer& player = it->second;
        WeaponType weapon = player.currentWeapon;

        // Set cooldown for this weapon
        player.shootCooldown = Missile::getCooldown(weapon);

        float baseSpeed = Missile::getSpeed(weapon);
        float spawnX = static_cast<float>(player.x) + MISSILE_SPAWN_OFFSET_X;
        float spawnY = static_cast<float>(player.y) + MISSILE_SPAWN_OFFSET_Y;

        switch (weapon) {
            case WeaponType::Standard: {
                // Single straight shot
                Missile missile{
                    .id = _nextMissileId++,
                    .owner_id = playerId,
                    .x = spawnX,
                    .y = spawnY,
                    .velocityX = baseSpeed,
                    .velocityY = 0.0f,
                    .weaponType = weapon,
                    .targetEnemyId = 0
                };
                _missiles[missile.id] = missile;
                spawnedIds.push_back(missile.id);
                break;
            }

            case WeaponType::Spread: {
                // 3 shots in a spread pattern
                float angles[] = {-15.0f, 0.0f, 15.0f};  // degrees
                for (float angle : angles) {
                    float radians = angle * 3.14159f / 180.0f;
                    Missile missile{
                        .id = _nextMissileId++,
                        .owner_id = playerId,
                        .x = spawnX,
                        .y = spawnY,
                        .velocityX = baseSpeed * std::cos(radians),
                        .velocityY = baseSpeed * std::sin(radians),
                        .weaponType = weapon,
                        .targetEnemyId = 0
                    };
                    _missiles[missile.id] = missile;
                    spawnedIds.push_back(missile.id);
                }
                break;
            }

            case WeaponType::Laser: {
                // Fast narrow beam
                Missile missile{
                    .id = _nextMissileId++,
                    .owner_id = playerId,
                    .x = spawnX,
                    .y = spawnY,
                    .velocityX = baseSpeed,
                    .velocityY = 0.0f,
                    .weaponType = weapon,
                    .targetEnemyId = 0
                };
                _missiles[missile.id] = missile;
                spawnedIds.push_back(missile.id);
                break;
            }

            case WeaponType::Missile: {
                // Homing missile - find nearest enemy
                uint16_t nearestEnemy = 0;
                float nearestDist = 99999.0f;

                // Check regular enemies
                for (const auto& [enemyId, enemy] : _enemies) {
                    float dx = enemy.x - spawnX;
                    float dy = enemy.y - spawnY;
                    float dist = std::sqrt(dx * dx + dy * dy);
                    if (dist < nearestDist) {
                        nearestDist = dist;
                        nearestEnemy = enemyId;
                    }
                }

                // Also check boss
                if (_boss.has_value() && _boss->isActive) {
                    float dx = _boss->x - spawnX;
                    float dy = _boss->y - spawnY;
                    float dist = std::sqrt(dx * dx + dy * dy);
                    if (dist < nearestDist) {
                        nearestDist = dist;
                        nearestEnemy = _boss->id;
                    }
                }

                Missile missile{
                    .id = _nextMissileId++,
                    .owner_id = playerId,
                    .x = spawnX,
                    .y = spawnY,
                    .velocityX = baseSpeed,
                    .velocityY = 0.0f,
                    .weaponType = weapon,
                    .targetEnemyId = nearestEnemy
                };
                _missiles[missile.id] = missile;
                spawnedIds.push_back(missile.id);
                break;
            }

            default:
                break;
        }

        return spawnedIds;
    }
}
