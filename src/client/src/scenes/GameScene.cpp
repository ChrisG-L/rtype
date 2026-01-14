/*
** EPITECH PROJECT, 2025
** rtype [WSL : Ubuntu-24.04]
** File description:
** GameScene
*/

#include "scenes/GameScene.hpp"
#include "scenes/SceneManager.hpp"
#include "scenes/MainMenuScene.hpp"
#include "core/Logger.hpp"
#include "events/Event.hpp"
#include "utils/Vecs.hpp"
#include "accessibility/AccessibilityConfig.hpp"
#include "Protocol.hpp"  // For InputKeys
#include <variant>
#include <algorithm>
#include <cmath>
#include <cstdio>  // For std::snprintf in renderScoreHUD

GameScene::GameScene(uint16_t roomGameSpeedPercent)
    : _roomGameSpeedMultiplier(static_cast<float>(std::clamp(roomGameSpeedPercent, static_cast<uint16_t>(50), static_cast<uint16_t>(200))) / 100.0f)
{
    client::logging::Logger::getSceneLogger()->debug("GameScene created with room game speed {}% (multiplier: {:.2f})",
        roomGameSpeedPercent, _roomGameSpeedMultiplier);
}

GameScene::GameScene(uint16_t roomGameSpeedPercent,
                     const std::vector<client::network::ChatMessageInfo>& initialChatMessages)
    : _roomGameSpeedMultiplier(static_cast<float>(std::clamp(roomGameSpeedPercent, static_cast<uint16_t>(50), static_cast<uint16_t>(200))) / 100.0f)
{
    client::logging::Logger::getSceneLogger()->debug("GameScene created with room game speed {}% and {} lobby messages",
        roomGameSpeedPercent, initialChatMessages.size());

    // Initialize chat display messages from lobby history
    // Mark all as expired so they appear in the collapsed/archived list
    for (const auto& msg : initialChatMessages) {
        ChatDisplayMessage displayMsg;
        displayMsg.displayName = msg.displayName;
        displayMsg.message = msg.message;
        displayMsg.displayTime = 0.0f;
        displayMsg.expired = true;  // Already in history
        _chatDisplayMessages.push_back(std::move(displayMsg));
    }
}

void GameScene::loadAssets()
{
    if (_assetsLoaded || !_context.window) return;

    // Load all 6 ship skin textures
    for (int i = 1; i <= 6; ++i) {
        std::string key = "ship" + std::to_string(i);
        std::string path = "assets/spaceship/Ship" + std::to_string(i) + ".png";
        _context.window->loadTexture(key, path);
    }

    // Keep backward compatibility with old texture key
    _context.window->loadTexture(SHIP_TEXTURE_KEY, "assets/spaceship/Ship1.png");
    _context.window->loadTexture(MISSILE_TEXTURE_KEY, "assets/spaceship/missile.png");

    // Load enemy textures per type
    // TODO: Replace with proper enemy sprites when assets are created
    // Expected assets:
    //   - assets/enemies/basic.png (gray standard enemy)
    //   - assets/enemies/tracker.png (red aggressive enemy)
    //   - assets/enemies/zigzag.png (green erratic enemy)
    //   - assets/enemies/fast.png (yellow fast enemy)
    //   - assets/enemies/bomber.png (purple heavy enemy)
    // For now, use different ship skins as visual distinction
    _context.window->loadTexture(ENEMY_BASIC_KEY, "assets/spaceship/Ship2.png");    // Different ship for basic
    _context.window->loadTexture(ENEMY_TRACKER_KEY, "assets/spaceship/Ship3.png");  // Different ship for tracker
    _context.window->loadTexture(ENEMY_ZIGZAG_KEY, "assets/spaceship/Ship4.png");   // Different ship for zigzag
    _context.window->loadTexture(ENEMY_FAST_KEY, "assets/spaceship/Ship5.png");     // Different ship for fast
    _context.window->loadTexture(ENEMY_BOMBER_KEY, "assets/spaceship/Ship6.png");   // Different ship for bomber
    _context.window->loadTexture(ENEMY_TEXTURE_KEY, "assets/spaceship/Ship2.png");  // Fallback

    // Load boss texture (use Ship6 as placeholder until proper boss sprite exists)
    _context.window->loadTexture(BOSS_TEXTURE_KEY, "assets/spaceship/Ship6.png");

    _context.window->loadFont(FONT_KEY, "assets/fonts/ARIA.TTF");
    _assetsLoaded = true;

    client::logging::Logger::getSceneLogger()->debug("GameScene assets loaded");
}

void GameScene::initStars()
{
    if (_starsInitialized) return;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> xDist(0.0f, SCREEN_WIDTH);
    std::uniform_real_distribution<float> yDist(0.0f, SCREEN_HEIGHT);
    std::uniform_real_distribution<float> speedDist(STAR_MIN_SPEED, STAR_MAX_SPEED);
    std::uniform_real_distribution<float> sizeDist(1.0f, 3.0f);
    std::uniform_int_distribution<int> brightnessDist(100, 255);

    _stars.reserve(STAR_COUNT);
    for (size_t i = 0; i < STAR_COUNT; ++i) {
        Star star{
            .x = xDist(gen),
            .y = yDist(gen),
            .speed = speedDist(gen),
            .size = sizeDist(gen),
            .brightness = static_cast<uint8_t>(brightnessDist(gen))
        };
        _stars.push_back(star);
    }

    _starsInitialized = true;
    client::logging::Logger::getSceneLogger()->debug("Stars initialized: {}", STAR_COUNT);
}

void GameScene::initAudio()
{
    if (_audioInitialized) return;

    auto& audioMgr = audio::AudioManager::getInstance();

    if (!audioMgr.isInitialized()) {
        if (!audioMgr.init()) {
            client::logging::Logger::getSceneLogger()->warn("Failed to initialize audio system");
            _audioInitialized = true;
            return;
        }
    }

    audioMgr.loadMusic("bgm", "assets/audio/background.ogg");

    audioMgr.loadSound("shoot", "assets/audio/shoot.mp3");
    audioMgr.loadSound("hit", "assets/audio/hit.mp3");

    audioMgr.playMusic("bgm", -1);

    _audioInitialized = true;
    client::logging::Logger::getSceneLogger()->debug("Audio initialized");
}

void GameScene::initVoiceChat()
{
    if (_voiceChatInitialized) return;

    auto& voiceMgr = audio::VoiceChatManager::getInstance();

    if (!voiceMgr.isInitialized()) {
        if (!voiceMgr.init()) {
            client::logging::Logger::getSceneLogger()->warn("Failed to initialize voice chat system");
            _voiceChatInitialized = true;
            return;
        }
    }

    // Get server host from TCP client
    if (_context.tcpClient && _context.tcpClient->isConnected()) {
        std::string serverHost = _context.tcpClient->getLastHost();

        // Connect to voice server
        if (voiceMgr.connect(serverHost, VOICE_UDP_PORT)) {
            // Get room code and session token for voice channel
            auto roomCodeOpt = _context.tcpClient->getCurrentRoomCode();
            auto tokenOpt = _context.tcpClient->getSessionToken();

            if (roomCodeOpt && tokenOpt) {
                voiceMgr.joinVoiceChannel(*tokenOpt, *roomCodeOpt);
                client::logging::Logger::getSceneLogger()->info(
                    "Voice chat connected for room '{}'", *roomCodeOpt);
            }
        }
    }

    _voiceChatInitialized = true;
    client::logging::Logger::getSceneLogger()->debug("Voice chat initialized");
}

void GameScene::handleEvent(const events::Event& event)
{
    // Handle chat input when open
    if (_chatInputOpen && _chatInput) {
        // Skip the first TextEntered event after opening chat (avoids 'T' appearing in input)
        if (_skipNextTextEntered && std::holds_alternative<events::TextEntered>(event)) {
            _skipNextTextEntered = false;
            return;
        }

        _chatInput->handleEvent(event);

        if (std::holds_alternative<events::KeyPressed>(event)) {
            auto& key = std::get<events::KeyPressed>(event);
            if (key.key == events::Key::Enter) {
                onSendChatMessage();
                return;
            } else if (key.key == events::Key::Escape) {
                _chatInputOpen = false;
                _chatInput->clear();
                return;
            }
        }
        return;  // Don't process other input when chat is open
    }

    if (std::holds_alternative<events::KeyPressed>(event)) {
        auto& key = std::get<events::KeyPressed>(event);
        _keysPressed.insert(key.key);

        // If kicked or dead, any key returns to main menu
        if ((_wasKicked || _context.udpClient->isLocalPlayerDead()) && _sceneManager) {
            // Leave the room before returning to main menu
            if (_context.tcpClient && _context.tcpClient->isConnected()) {
                _context.tcpClient->leaveRoom();
            }
            _sceneManager->changeScene(std::make_unique<MainMenuScene>());
            return;
        }

        // Toggle chat with T key
        if (key.key == events::Key::T && !_context.udpClient->isLocalPlayerDead() && !_wasKicked) {
            _chatInputOpen = true;
            _skipNextTextEntered = true;  // Prevent 'T' from appearing in chat input
            if (_chatInput) {
                _chatInput->setFocused(true);
            }
            return;
        }

        // Toggle chat expansion with O key
        if (key.key == events::Key::O && !_chatDisplayMessages.empty()) {
            _chatExpanded = !_chatExpanded;
            return;
        }

        // Push-to-Talk (configurable key)
        auto& accessConfig = accessibility::AccessibilityConfig::getInstance();
        if (accessConfig.isActionKey(accessibility::GameAction::PushToTalk, key.key) && !_chatInputOpen) {
            auto& voiceMgr = audio::VoiceChatManager::getInstance();
            if (voiceMgr.isConnected() &&
                voiceMgr.getVoiceMode() == audio::VoiceChatManager::VoiceMode::PushToTalk) {
                voiceMgr.startTalking();
            }
        }
    } else if (std::holds_alternative<events::KeyReleased>(event)) {
        auto& key = std::get<events::KeyReleased>(event);
        _keysPressed.erase(key.key);

        // Release Push-to-Talk (configurable key)
        auto& accessConfig = accessibility::AccessibilityConfig::getInstance();
        if (accessConfig.isActionKey(accessibility::GameAction::PushToTalk, key.key)) {
            auto& voiceMgr = audio::VoiceChatManager::getInstance();
            if (voiceMgr.isConnected()) {
                voiceMgr.stopTalking();
            }
        }
    }
}

void GameScene::processUDPEvents()
{
    if (!_context.udpClient) return;

    while (auto eventOpt = _context.udpClient->pollEvent()) {
        std::visit([this]([[maybe_unused]] auto&& event) {
            // UDP events are handled by UDPClient internally
            // Kick notifications are now handled via TCP (TCPPlayerKickedEvent)
        }, *eventOpt);
    }

    // Client-side prediction reconciliation
    // After processing events, reconcile our predicted position with server state
    auto localId = _context.udpClient->getLocalPlayerId();
    if (localId) {
        auto players = _context.udpClient->getPlayers();
        for (const auto& p : players) {
            if (p.id == *localId) {
                reconcileWithServer(p);
                break;
            }
        }
    }
}

void GameScene::applyInputToPosition(float& x, float& y, uint16_t keys, float dt)
{
    float speed = MOVE_SPEED * dt;

    if (keys & InputKeys::UP)    y -= speed;
    if (keys & InputKeys::DOWN)  y += speed;
    if (keys & InputKeys::LEFT)  x -= speed;
    if (keys & InputKeys::RIGHT) x += speed;

    // Clamp to screen bounds (same as server)
    x = std::clamp(x, 0.0f, SCREEN_WIDTH - SHIP_WIDTH);
    y = std::clamp(y, 0.0f, SCREEN_HEIGHT - SHIP_HEIGHT);
}

void GameScene::reconcileWithServer(const client::network::NetworkPlayer& serverState)
{
    // Remove inputs that have been acknowledged by the server
    while (!_pendingInputs.empty() &&
           _pendingInputs.front().sequenceNum <= serverState.lastAckedInputSeq) {
        _pendingInputs.pop_front();
    }

    // Start from server's authoritative position
    _predictedX = static_cast<float>(serverState.x);
    _predictedY = static_cast<float>(serverState.y);

    // Replay all pending inputs that haven't been acknowledged yet
    for (const auto& input : _pendingInputs) {
        applyInputToPosition(_predictedX, _predictedY, input.keys, input.deltaTime);
    }
}

void GameScene::update(float deltatime)
{
    if (!_context.udpClient) return;

    if (!_assetsLoaded) {
        loadAssets();
    }

    if (!_starsInitialized) {
        initStars();
    }

    if (!_audioInitialized) {
        initAudio();
    }

    if (!_chatUIInitialized) {
        initChatUI();
    }

    if (!_voiceChatInitialized) {
        initVoiceChat();
    }

    // Update voice chat (process incoming audio)
    audio::VoiceChatManager::getInstance().update();

    // Process UDP events (check for kick)
    processUDPEvents();

    // Process TCP events (chat messages)
    processTCPEvents();

    // Update chat message display timers (mark as expired when timer reaches 0)
    // Last ALWAYS_VISIBLE_MESSAGES messages never expire
    size_t msgCount = _chatDisplayMessages.size();
    for (size_t i = 0; i < msgCount; ++i) {
        auto& msg = _chatDisplayMessages[i];

        // Skip if already expired
        if (msg.expired) continue;

        // Last ALWAYS_VISIBLE_MESSAGES never expire
        // If we have <= ALWAYS_VISIBLE_MESSAGES, none expire
        if (msgCount <= ALWAYS_VISIBLE_MESSAGES || i >= msgCount - ALWAYS_VISIBLE_MESSAGES) continue;

        if (msg.displayTime > 0) {
            msg.displayTime -= deltatime;
            if (msg.displayTime <= 0) {
                msg.expired = true;
            }
        }
    }

    // Remove oldest expired messages if we exceed MAX_CHAT_DISPLAY_MESSAGES
    while (_chatDisplayMessages.size() > MAX_CHAT_DISPLAY_MESSAGES) {
        _chatDisplayMessages.erase(_chatDisplayMessages.begin());
    }

    // Update chat input if open
    if (_chatInputOpen && _chatInput) {
        _chatInput->update(deltatime);
    }

    // If kicked, don't update gameplay
    if (_wasKicked) {
        return;
    }

    // Use room game speed instead of per-player AccessibilityConfig
    auto& accessConfig = accessibility::AccessibilityConfig::getInstance();
    float adjustedDeltaTime = deltatime * _roomGameSpeedMultiplier;

    for (auto& star : _stars) {
        star.x -= star.speed * adjustedDeltaTime;
        if (star.x < 0) {
            star.x = SCREEN_WIDTH;
        }
    }

    if (_context.udpClient->isLocalPlayerDead()) {
        return;
    }

    // Build input keys bitfield from pressed keys
    uint16_t inputKeys = 0;
    bool shootPressed = false;

    for (const auto& key : _keysPressed) {
        if (accessConfig.isActionKey(accessibility::GameAction::MoveUp, key)) {
            inputKeys |= InputKeys::UP;
        }
        if (accessConfig.isActionKey(accessibility::GameAction::MoveDown, key)) {
            inputKeys |= InputKeys::DOWN;
        }
        if (accessConfig.isActionKey(accessibility::GameAction::MoveLeft, key)) {
            inputKeys |= InputKeys::LEFT;
        }
        if (accessConfig.isActionKey(accessibility::GameAction::MoveRight, key)) {
            inputKeys |= InputKeys::RIGHT;
        }
        if (accessConfig.isActionKey(accessibility::GameAction::Shoot, key)) {
            shootPressed = true;
        }
        // Weapon switching: Q = previous, E = next
        if (key == events::Key::Q) {
            inputKeys |= InputKeys::WEAPON_PREV;
        }
        if (key == events::Key::E) {
            inputKeys |= InputKeys::WEAPON_NEXT;
        }
    }

    // Client-side prediction: send input with sequence, store pending, apply locally
    uint16_t seq = _nextInputSeq++;
    _context.udpClient->sendPlayerInput(inputKeys, seq);

    // Store pending input for reconciliation
    _pendingInputs.push_back({seq, inputKeys, adjustedDeltaTime});

    // Apply prediction locally (immediate response)
    applyInputToPosition(_predictedX, _predictedY, inputKeys, adjustedDeltaTime);

    // Handle shooting (with client-side cooldown for feedback)
    if (_shootCooldown > 0.0f) {
        _shootCooldown -= adjustedDeltaTime;
    }

    if (shootPressed && _shootCooldown <= 0.0f) {
        _context.udpClient->shootMissile();
        _shootCooldown = SHOOT_COOLDOWN_TIME;

        audio::AudioManager::getInstance().playSound("shoot");

        client::logging::Logger::getSceneLogger()->debug("Missile fired!");
    }
}

void GameScene::renderBackground()
{
    _context.window->drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, {5, 5, 20, 255});

    for (const auto& star : _stars) {
        _context.window->drawRect(
            star.x,
            star.y,
            star.size,
            star.size,
            {star.brightness, star.brightness, star.brightness, 255}
        );
    }
}

void GameScene::renderHUD()
{
    auto localId = _context.udpClient->getLocalPlayerId();
    if (!localId) return;

    auto players = _context.udpClient->getPlayers();
    uint8_t localHealth = MAX_HEALTH;

    for (const auto& player : players) {
        if (player.id == *localId) {
            localHealth = player.health;
            break;
        }
    }

    float healthRatio = static_cast<float>(localHealth) / static_cast<float>(MAX_HEALTH);
    float currentBarWidth = HUD_HEALTH_BAR_WIDTH * healthRatio;

    _context.window->drawRect(
        HUD_MARGIN,
        HUD_MARGIN,
        HUD_HEALTH_BAR_WIDTH,
        HUD_HEALTH_BAR_HEIGHT,
        {40, 40, 40, 255}
    );

    auto& accessConfig = accessibility::AccessibilityConfig::getInstance();
    rgba healthColor;
    if (healthRatio > 0.6f) {
        auto c = accessConfig.getHealthHighColor();
        healthColor = {c.r, c.g, c.b, c.a};
    } else if (healthRatio > 0.3f) {
        auto c = accessConfig.getHealthMediumColor();
        healthColor = {c.r, c.g, c.b, c.a};
    } else {
        auto c = accessConfig.getHealthLowColor();
        healthColor = {c.r, c.g, c.b, c.a};
    }

    if (currentBarWidth > 0) {
        _context.window->drawRect(
            HUD_MARGIN,
            HUD_MARGIN,
            currentBarWidth,
            HUD_HEALTH_BAR_HEIGHT,
            healthColor
        );
    }
}

void GameScene::renderScoreHUD()
{
    auto localId = _context.udpClient->getLocalPlayerId();
    if (!localId) return;

    auto players = _context.udpClient->getPlayers();

    // Find local player's score data
    uint32_t score = 0;
    uint16_t kills = 0;
    uint8_t combo = 10;  // 1.0x default

    for (const auto& p : players) {
        if (p.id == *localId) {
            score = p.score;
            kills = p.kills;
            combo = p.combo;
            break;
        }
    }

    // Get wave number
    uint16_t waveNumber = _context.udpClient->getWaveNumber();

    // === Score display (top-right) ===
    float scoreX = SCREEN_WIDTH - 280.0f;
    float scoreY = 20.0f;

    // Score background
    _context.window->drawRect(scoreX - 10.0f, scoreY - 5.0f, 270.0f, 75.0f, {20, 20, 40, 180});

    // Score value
    std::string scoreText = "SCORE: " + std::to_string(score);
    _context.window->drawText(FONT_KEY, scoreText, scoreX, scoreY, 22, {255, 255, 255, 255});

    // Kills
    std::string killsText = "KILLS: " + std::to_string(kills);
    _context.window->drawText(FONT_KEY, killsText, scoreX, scoreY + 28.0f, 16, {200, 200, 200, 255});

    // Combo (only show if > 1.0x)
    if (combo > 10) {
        float comboValue = static_cast<float>(combo) / 10.0f;

        // Format combo (e.g., "x1.5" or "x2.0")
        char comboBuf[16];
        std::snprintf(comboBuf, sizeof(comboBuf), "COMBO x%.1f", comboValue);
        std::string comboText = comboBuf;

        // Color based on combo level
        rgba comboColor;
        if (combo >= 25) {
            comboColor = {255, 100, 100, 255};  // Red for high combo (2.5x+)
        } else if (combo >= 20) {
            comboColor = {255, 200, 50, 255};   // Gold for medium-high (2.0x+)
        } else {
            comboColor = {100, 255, 100, 255};  // Green for building combo
        }

        _context.window->drawText(FONT_KEY, comboText, scoreX + 120.0f, scoreY + 28.0f, 16, comboColor);
    }

    // === Wave display (top-center) ===
    if (waveNumber > 0) {
        std::string waveText = "WAVE " + std::to_string(waveNumber);
        float waveX = (SCREEN_WIDTH - 100.0f) / 2.0f;
        float waveY = 20.0f;

        // Wave background
        _context.window->drawRect(waveX - 15.0f, waveY - 5.0f, 130.0f, 35.0f, {20, 20, 40, 180});

        _context.window->drawText(FONT_KEY, waveText, waveX, waveY, 22, {255, 220, 100, 255});
    }
}

void GameScene::renderWeaponHUD()
{
    auto localId = _context.udpClient->getLocalPlayerId();
    if (!localId) return;

    auto players = _context.udpClient->getPlayers();

    // Find local player's weapon
    uint8_t currentWeapon = 0;
    for (const auto& p : players) {
        if (p.id == *localId) {
            currentWeapon = p.currentWeapon;
            break;
        }
    }

    // Weapon names
    static const char* weaponNames[] = {"STANDARD", "SPREAD", "LASER", "MISSILE"};
    static const rgba weaponColors[] = {
        {180, 180, 180, 255},  // Standard - gray
        {100, 200, 255, 255},  // Spread - cyan
        {255, 100, 100, 255},  // Laser - red
        {100, 255, 100, 255}   // Missile - green
    };

    // Clamp weapon index
    if (currentWeapon >= 4) currentWeapon = 0;

    // Display position (bottom-left, above chat)
    float hudX = 20.0f;
    float hudY = SCREEN_HEIGHT - 150.0f;

    // Background
    _context.window->drawRect(hudX - 5.0f, hudY - 5.0f, 180.0f, 50.0f, {20, 20, 40, 200});

    // Label
    _context.window->drawText(FONT_KEY, "WEAPON", hudX, hudY, 12, {150, 150, 150, 255});

    // Current weapon name with color
    _context.window->drawText(FONT_KEY, weaponNames[currentWeapon], hudX, hudY + 18.0f, 18, weaponColors[currentWeapon]);

    // Control hint
    _context.window->drawText(FONT_KEY, "[Q/E] Switch", hudX + 90.0f, hudY + 22.0f, 10, {100, 100, 120, 255});
}

void GameScene::renderPlayers()
{
    auto localId = _context.udpClient->getLocalPlayerId();
    auto players = _context.udpClient->getPlayers();

    for (const auto& player : players) {
        if (!player.alive) {
            continue;
        }

        float px, py;
        if (localId && player.id == *localId) {
            // Local player: use predicted position (client-side prediction)
            px = _predictedX;
            py = _predictedY;
        } else {
            // Other players: use server position
            px = static_cast<float>(player.x);
            py = static_cast<float>(player.y);
        }

        if (_assetsLoaded) {
            // Use player's ship skin (clamp to valid range 1-6)
            uint8_t skinId = std::clamp(player.shipSkin, static_cast<uint8_t>(1), static_cast<uint8_t>(6));
            std::string textureKey = "ship" + std::to_string(skinId);

            _context.window->drawSprite(
                textureKey,
                px,
                py,
                SHIP_WIDTH,
                SHIP_HEIGHT
            );

            std::string label;
            if (localId && player.id == *localId) {
                label = "You";
            } else {
                label = "P" + std::to_string(player.id);
            }
            float labelX = px + (SHIP_WIDTH / 2.0f) - 10.0f;
            float labelY = py - 18.0f;
            _context.window->drawText(FONT_KEY, label, labelX, labelY, LABEL_FONT_SIZE, {255, 255, 255, 255});
        } else {
            auto& accessConfig = accessibility::AccessibilityConfig::getInstance();
            rgba color;
            if (localId && player.id == *localId) {
                auto c = accessConfig.getPlayerColor();
                color = {c.r, c.g, c.b, c.a};
            } else {
                auto c = accessConfig.getOtherPlayerColor();
                color = {c.r, c.g, c.b, c.a};
            }

            _context.window->drawRect(
                px,
                py,
                SHIP_WIDTH,
                SHIP_HEIGHT,
                color
            );
        }
    }
}

void GameScene::renderMissiles()
{
    auto missiles = _context.udpClient->getMissiles();

    for (const auto& missile : missiles) {
        if (_assetsLoaded) {
            _context.window->drawSprite(
                MISSILE_TEXTURE_KEY,
                static_cast<float>(missile.x),
                static_cast<float>(missile.y),
                MISSILE_WIDTH,
                MISSILE_HEIGHT
            );
        } else {
            auto& accessConfig = accessibility::AccessibilityConfig::getInstance();
            auto c = accessConfig.getPlayerMissileColor();
            _context.window->drawRect(
                static_cast<float>(missile.x),
                static_cast<float>(missile.y),
                MISSILE_WIDTH,
                MISSILE_HEIGHT,
                {c.r, c.g, c.b, c.a}
            );
        }
    }
}

std::string GameScene::getEnemyTextureKey(uint8_t enemyType) const
{
    // EnemyType enum from GameWorld.hpp:
    // Basic = 0, Tracker = 1, Zigzag = 2, Fast = 3, Bomber = 4
    switch (enemyType) {
        case 0: return ENEMY_BASIC_KEY;    // Basic
        case 1: return ENEMY_TRACKER_KEY;  // Tracker
        case 2: return ENEMY_ZIGZAG_KEY;   // Zigzag
        case 3: return ENEMY_FAST_KEY;     // Fast
        case 4: return ENEMY_BOMBER_KEY;   // Bomber
        default: return ENEMY_BASIC_KEY;   // Fallback to basic
    }
}

void GameScene::renderEnemies()
{
    auto enemies = _context.udpClient->getEnemies();

    for (const auto& enemy : enemies) {
        if (_assetsLoaded) {
            // Use different texture based on enemy type
            std::string textureKey = getEnemyTextureKey(enemy.enemy_type);

            // Adjust size based on enemy type (bomber is larger)
            float width = ENEMY_WIDTH;
            float height = ENEMY_HEIGHT;
            if (enemy.enemy_type == 4) {  // Bomber
                width = ENEMY_WIDTH * 1.5f;
                height = ENEMY_HEIGHT * 1.5f;
            } else if (enemy.enemy_type == 3) {  // Fast (smaller)
                width = ENEMY_WIDTH * 0.8f;
                height = ENEMY_HEIGHT * 0.8f;
            }

            _context.window->drawSprite(
                textureKey,
                static_cast<float>(enemy.x),
                static_cast<float>(enemy.y),
                width,
                height
            );
        } else {
            // Accessibility mode: use different colors per enemy type
            rgba color;
            switch (enemy.enemy_type) {
                case 0:  // Basic - gray
                    color = {150, 150, 150, 255};
                    break;
                case 1:  // Tracker - red (aggressive)
                    color = {255, 80, 80, 255};
                    break;
                case 2:  // Zigzag - green (erratic)
                    color = {80, 255, 80, 255};
                    break;
                case 3:  // Fast - yellow (speed)
                    color = {255, 255, 80, 255};
                    break;
                case 4:  // Bomber - purple (heavy)
                    color = {180, 80, 255, 255};
                    break;
                default:
                    auto& accessConfig = accessibility::AccessibilityConfig::getInstance();
                    auto c = accessConfig.getEnemyColor();
                    color = {c.r, c.g, c.b, c.a};
                    break;
            }

            float width = ENEMY_WIDTH;
            float height = ENEMY_HEIGHT;
            if (enemy.enemy_type == 4) {  // Bomber (larger)
                width = ENEMY_WIDTH * 1.5f;
                height = ENEMY_HEIGHT * 1.5f;
            } else if (enemy.enemy_type == 3) {  // Fast (smaller)
                width = ENEMY_WIDTH * 0.8f;
                height = ENEMY_HEIGHT * 0.8f;
            }

            _context.window->drawRect(
                static_cast<float>(enemy.x),
                static_cast<float>(enemy.y),
                width,
                height,
                color
            );
        }
    }
}

void GameScene::renderEnemyMissiles()
{
    auto enemyMissiles = _context.udpClient->getEnemyMissiles();

    auto& accessConfig = accessibility::AccessibilityConfig::getInstance();
    auto c = accessConfig.getEnemyMissileColor();

    for (const auto& missile : enemyMissiles) {
        _context.window->drawRect(
            static_cast<float>(missile.x),
            static_cast<float>(missile.y),
            ENEMY_MISSILE_WIDTH,
            ENEMY_MISSILE_HEIGHT,
            {c.r, c.g, c.b, c.a}
        );
    }
}

void GameScene::renderDeathScreen()
{
    _context.window->drawRect(0, 0, SCREEN_WIDTH + 20, SCREEN_HEIGHT + 20, {0, 0, 0, 200});

    const std::string gameOverText = "GAME OVER";
    float textX = SCREEN_WIDTH / 2.0f - 110.0f;
    float textY = SCREEN_HEIGHT / 2.0f - 70.0f;

    if (_assetsLoaded) {
        _context.window->drawText(FONT_KEY, gameOverText, textX, textY, 48, {255, 50, 50, 255});

        const std::string instructionText = "You have been eliminated";
        float instrX = SCREEN_WIDTH / 2.0f - 140.0f;
        float instrY = textY + 70.0f;
        _context.window->drawText(FONT_KEY, instructionText, instrX, instrY, 24, {200, 200, 200, 255});

        const std::string hintText = "Press any key to return to menu";
        float hintX = SCREEN_WIDTH / 2.0f - 150.0f;
        float hintY = instrY + 50.0f;
        _context.window->drawText(FONT_KEY, hintText, hintX, hintY, 18, {150, 150, 150, 255});
    } else {
        _context.window->drawRect(textX, textY, 200.0f, 60.0f, {255, 50, 50, 255});
    }
}

void GameScene::renderKickedScreen()
{
    _context.window->drawRect(0, 0, SCREEN_WIDTH + 20, SCREEN_HEIGHT + 20, {0, 0, 0, 200});

    const std::string kickedText = "KICKED";
    float textX = SCREEN_WIDTH / 2.0f - 80.0f;
    float textY = SCREEN_HEIGHT / 2.0f - 70.0f;

    if (_assetsLoaded) {
        _context.window->drawText(FONT_KEY, kickedText, textX, textY, 48, {255, 150, 50, 255});

        const std::string instructionText = "You have been kicked from the game";
        float instrX = SCREEN_WIDTH / 2.0f - 180.0f;
        float instrY = textY + 70.0f;
        _context.window->drawText(FONT_KEY, instructionText, instrX, instrY, 24, {200, 200, 200, 255});

        // Display kick reason if available
        if (!_kickReason.empty()) {
            std::string reasonText = "Reason: " + _kickReason;
            float reasonX = SCREEN_WIDTH / 2.0f - 150.0f;
            float reasonY = instrY + 40.0f;
            _context.window->drawText(FONT_KEY, reasonText, reasonX, reasonY, 20, {255, 200, 100, 255});
        }

        const std::string hintText = "Press any key to return to menu";
        float hintX = SCREEN_WIDTH / 2.0f - 150.0f;
        float hintY = _kickReason.empty() ? (instrY + 50.0f) : (instrY + 90.0f);
        _context.window->drawText(FONT_KEY, hintText, hintX, hintY, 18, {150, 150, 150, 255});
    } else {
        _context.window->drawRect(textX, textY, 160.0f, 60.0f, {255, 150, 50, 255});
    }
}

void GameScene::renderVoiceIndicator()
{
    if (!_context.window) return;

    auto& voiceMgr = audio::VoiceChatManager::getInstance();
    if (!voiceMgr.isConnected()) return;

    float indicatorX = SCREEN_WIDTH - 200.0f;
    float indicatorY = 20.0f;

    // Show muted indicator
    if (voiceMgr.isMuted()) {
        _context.window->drawRect(indicatorX, indicatorY, 80, 24, {100, 100, 100, 220});
        _context.window->drawText(FONT_KEY, "MUTED", indicatorX + 10, indicatorY + 4, 14, {200, 200, 200, 255});
        indicatorY += 30.0f;
    }
    // Show "MIC" indicator when we're talking
    else if (voiceMgr.isTalking()) {
        _context.window->drawRect(indicatorX, indicatorY, 60, 24, {180, 40, 40, 220});
        _context.window->drawText(FONT_KEY, "MIC", indicatorX + 10, indicatorY + 4, 14, {255, 255, 255, 255});
        indicatorY += 30.0f;
    }

    // Show who is speaking
    auto speakers = voiceMgr.getActiveSpeakers();
    for (uint8_t speakerId : speakers) {
        _context.window->drawRect(indicatorX, indicatorY, 120, 22, {40, 120, 40, 200});
        std::string label = "P" + std::to_string(speakerId) + " speaking";
        _context.window->drawText(FONT_KEY, label, indicatorX + 8, indicatorY + 4, 12, {255, 255, 255, 255});
        indicatorY += 26.0f;
    }

    // Show voice mode hint in bottom-right
    if (!voiceMgr.isTalking() && speakers.empty() && !voiceMgr.isMuted()) {
        float hintX = SCREEN_WIDTH - 140.0f;
        float hintY = SCREEN_HEIGHT - 30.0f;

        bool isVAD = (voiceMgr.getVoiceMode() == audio::VoiceChatManager::VoiceMode::VoiceActivity);
        std::string hint = isVAD ? "Voice Activity" : "[V] Push-to-Talk";
        _context.window->drawText(FONT_KEY, hint, hintX, hintY, 12, {100, 100, 120, 255});
    }
}

void GameScene::render()
{
    if (!_context.window || !_context.udpClient) return;

    renderBackground();
    renderEnemies();
    renderBoss();          // Boss sprite (Gameplay Phase 2)
    renderPlayers();
    renderMissiles();
    renderEnemyMissiles();
    renderHUD();
    renderScoreHUD();
    renderWeaponHUD();     // Weapon indicator (Gameplay Phase 2)
    renderBossHealthBar(); // Boss HP bar at top (Gameplay Phase 2)
    renderVoiceIndicator();

    // Always render chat overlay (even when dead)
    renderChatOverlay();

    if (_wasKicked) {
        renderKickedScreen();
    } else if (_context.udpClient->isLocalPlayerDead()) {
        renderDeathScreen();
    }
}

// ============================================================================
// Boss System (Gameplay Phase 2)
// ============================================================================

void GameScene::renderBoss()
{
    auto bossOpt = _context.udpClient->getBossState();
    if (!bossOpt || !bossOpt->is_active) return;

    const auto& boss = *bossOpt;

    if (_assetsLoaded) {
        _context.window->drawSprite(
            BOSS_TEXTURE_KEY,
            static_cast<float>(boss.x),
            static_cast<float>(boss.y),
            BOSS_WIDTH,
            BOSS_HEIGHT
        );
    } else {
        // Accessibility mode: purple color for boss
        rgba bossColor;
        switch (boss.phase) {
            case 0:  // Phase 1 - dark purple
                bossColor = {120, 40, 180, 255};
                break;
            case 1:  // Phase 2 - brighter purple
                bossColor = {160, 60, 220, 255};
                break;
            case 2:  // Phase 3 - red/purple (enraged)
                bossColor = {220, 40, 140, 255};
                break;
            default:
                bossColor = {140, 40, 200, 255};
                break;
        }

        _context.window->drawRect(
            static_cast<float>(boss.x),
            static_cast<float>(boss.y),
            BOSS_WIDTH,
            BOSS_HEIGHT,
            bossColor
        );
    }

    // Draw "BOSS" label above
    float labelX = static_cast<float>(boss.x) + BOSS_WIDTH / 2.0f - 25.0f;
    float labelY = static_cast<float>(boss.y) - 25.0f;
    _context.window->drawText(FONT_KEY, "BOSS", labelX, labelY, 18, {255, 50, 50, 255});
}

void GameScene::renderBossHealthBar()
{
    auto bossOpt = _context.udpClient->getBossState();
    if (!bossOpt || !bossOpt->is_active) return;

    const auto& boss = *bossOpt;

    // Large health bar at top of screen
    static constexpr float BAR_WIDTH = 600.0f;
    static constexpr float BAR_HEIGHT = 25.0f;
    float barX = (SCREEN_WIDTH - BAR_WIDTH) / 2.0f;
    float barY = 60.0f;

    // Background
    _context.window->drawRect(barX - 2, barY - 2, BAR_WIDTH + 4, BAR_HEIGHT + 4, {40, 40, 60, 220});

    // Health ratio
    float healthRatio = (boss.max_health > 0)
        ? static_cast<float>(boss.health) / static_cast<float>(boss.max_health)
        : 0.0f;
    float currentWidth = BAR_WIDTH * healthRatio;

    // Color based on phase
    rgba barColor;
    switch (boss.phase) {
        case 0:  // Phase 1 - purple
            barColor = {140, 60, 200, 255};
            break;
        case 1:  // Phase 2 - orange
            barColor = {255, 140, 40, 255};
            break;
        case 2:  // Phase 3 - red (enraged)
            barColor = {255, 40, 40, 255};
            break;
        default:
            barColor = {140, 60, 200, 255};
            break;
    }

    if (currentWidth > 0) {
        _context.window->drawRect(barX, barY, currentWidth, BAR_HEIGHT, barColor);
    }

    // Boss name / phase indicator
    std::string bossTitle = "BOSS";
    if (boss.phase > 0) {
        bossTitle += " - PHASE " + std::to_string(boss.phase + 1);
    }
    float titleX = barX;
    float titleY = barY - 22.0f;
    _context.window->drawText(FONT_KEY, bossTitle, titleX, titleY, 16, {255, 200, 100, 255});

    // Health numbers
    std::string hpText = std::to_string(boss.health) + " / " + std::to_string(boss.max_health);
    float hpX = barX + BAR_WIDTH - 120.0f;
    _context.window->drawText(FONT_KEY, hpText, hpX, titleY, 14, {200, 200, 200, 255});
}

// ============================================================================
// Chat System (Phase 2)
// ============================================================================

void GameScene::initChatUI()
{
    if (_chatUIInitialized || !_context.window) return;

    _chatInput = std::make_unique<ui::TextInput>(
        Vec2f{20.0f, SCREEN_HEIGHT - 50.0f},
        Vec2f{400.0f, 40.0f},
        "Press Enter to send...",
        FONT_KEY
    );
    _chatInput->setMaxLength(200);

    _chatUIInitialized = true;
}

void GameScene::processTCPEvents()
{
    if (!_context.tcpClient) return;

    while (auto eventOpt = _context.tcpClient->pollEvent()) {
        std::visit([this](auto&& event) {
            using T = std::decay_t<decltype(event)>;

            if constexpr (std::is_same_v<T, client::network::TCPChatMessageEvent>) {
                appendChatMessage(client::network::ChatMessageInfo{
                    event.displayName,
                    event.message,
                    event.timestamp
                });
            }
            else if constexpr (std::is_same_v<T, client::network::TCPPlayerKickedEvent>) {
                // We were kicked from the game (via TCP with reason)
                client::logging::Logger::getSceneLogger()->warn("Kicked from game: {}",
                    event.reason.empty() ? "No reason given" : event.reason);
                _wasKicked = true;
                _kickReason = event.reason;
            }
            // Ignore other TCP events during gameplay
        }, *eventOpt);
    }
}

void GameScene::onSendChatMessage()
{
    if (!_chatInput || !_chatInputOpen) return;

    std::string message = _chatInput->getText();
    if (message.empty()) {
        return;  // Don't close chat on empty message, user must press Escape
    }

    if (_context.tcpClient && _context.tcpClient->isConnected()) {
        _context.tcpClient->sendChatMessage(message);
    }

    _chatInput->clear();
    // Keep chat open for quick consecutive messages (Escape to close)
}

void GameScene::appendChatMessage(const client::network::ChatMessageInfo& msg)
{
    ChatDisplayMessage displayMsg;
    displayMsg.displayName = msg.displayName;
    displayMsg.message = msg.message;
    displayMsg.displayTime = CHAT_MESSAGE_DISPLAY_TIME;

    _chatDisplayMessages.push_back(std::move(displayMsg));

    // Keep only last MAX_CHAT_DISPLAY_MESSAGES
    while (_chatDisplayMessages.size() > MAX_CHAT_DISPLAY_MESSAGES) {
        _chatDisplayMessages.erase(_chatDisplayMessages.begin());
    }
}

void GameScene::renderChatOverlay()
{
    if (!_context.window) return;

    // Render chat messages in bottom-left corner
    float msgX = 20.0f;
    float msgY = SCREEN_HEIGHT - 100.0f;
    float msgSpacing = 28.0f;

    // Count expired (archived) messages
    size_t expiredCount = 0;
    for (const auto& msg : _chatDisplayMessages) {
        if (msg.expired) expiredCount++;
    }

    // Draw from bottom to top (most recent at bottom)
    for (int i = static_cast<int>(_chatDisplayMessages.size()) - 1; i >= 0; --i) {
        const auto& msg = _chatDisplayMessages[i];

        // Skip expired messages unless expanded
        if (msg.expired && !_chatExpanded) {
            continue;
        }

        // Calculate alpha based on remaining display time (fade out effect)
        uint8_t alpha = 255;
        if (!msg.expired && msg.displayTime < 2.0f) {
            alpha = static_cast<uint8_t>(255 * (msg.displayTime / 2.0f));
        }

        // Semi-transparent background for readability
        float textWidth = 400.0f;  // Approximate
        _context.window->drawRect(msgX - 5, msgY - 2, textWidth, 24, {0, 0, 0, static_cast<uint8_t>(alpha * 0.5f)});

        // Draw name in blue
        std::string nameText = msg.displayName + ": ";
        _context.window->drawText(FONT_KEY, nameText, msgX, msgY, 14, {100, 180, 255, alpha});

        // Draw message in white (truncate if too long)
        std::string displayText = msg.message;
        if (displayText.length() > 50) {
            displayText = displayText.substr(0, 47) + "...";
        }
        _context.window->drawText(FONT_KEY, displayText, msgX + 100, msgY, 14, {255, 255, 255, alpha});

        msgY -= msgSpacing;
    }

    // Show indicator for hidden (expired) messages
    if (expiredCount > 0 && !_chatExpanded) {
        std::string expandHint = "[+" + std::to_string(expiredCount) + " messages - O to show]";
        _context.window->drawRect(msgX - 5, msgY - 2, 260, 22, {40, 40, 60, 180});
        _context.window->drawText(FONT_KEY, expandHint, msgX, msgY, 12, {180, 180, 200, 255});
    } else if (_chatExpanded && expiredCount > 0) {
        // Show collapse hint when expanded
        _context.window->drawRect(msgX - 5, msgY - 2, 150, 22, {40, 40, 60, 180});
        _context.window->drawText(FONT_KEY, "[O to hide]", msgX, msgY, 12, {180, 180, 200, 255});
    }

    // Render chat input if open
    if (_chatInputOpen && _chatInput) {
        // Dark background for input
        _context.window->drawRect(15, SCREEN_HEIGHT - 55, 450, 50, {0, 0, 0, 200});
        _context.window->drawRect(15, SCREEN_HEIGHT - 55, 450, 2, {60, 100, 140, 255});

        _chatInput->render(*_context.window);

        // Hint text
        _context.window->drawText(FONT_KEY, "[Enter] Send  [Esc] Close",
            430, SCREEN_HEIGHT - 40, 12, {150, 150, 170, 255});
    } else if (!_chatDisplayMessages.empty()) {
        // Show hint to open chat
        _context.window->drawText(FONT_KEY, "[T] Chat",
            20, SCREEN_HEIGHT - 30, 12, {100, 100, 120, 255});
    }
}
