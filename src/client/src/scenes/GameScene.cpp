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

GameScene::GameScene(uint16_t roomGameSpeedPercent,
                     const std::vector<client::network::ChatMessageInfo>& initialChatMessages,
                     const std::unordered_map<uint8_t, std::string>& playerNames)
    : _roomGameSpeedMultiplier(static_cast<float>(std::clamp(roomGameSpeedPercent, static_cast<uint16_t>(50), static_cast<uint16_t>(200))) / 100.0f),
      _playerNames(playerNames)
{
    client::logging::Logger::getSceneLogger()->debug("GameScene created with room game speed {}%, {} lobby messages, {} player names",
        roomGameSpeedPercent, initialChatMessages.size(), playerNames.size());

    // Initialize chat display messages from lobby history
    for (const auto& msg : initialChatMessages) {
        ChatDisplayMessage displayMsg;
        displayMsg.displayName = msg.displayName;
        displayMsg.message = msg.message;
        displayMsg.displayTime = 0.0f;
        displayMsg.expired = true;
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
    //   - assets/enemies/pow_armor.png (orange power-up carrier)
    // For now, use different ship skins as visual distinction
    _context.window->loadTexture(ENEMY_BASIC_KEY, "assets/spaceship/Ship2.png");    // Different ship for basic
    _context.window->loadTexture(ENEMY_TRACKER_KEY, "assets/spaceship/Ship3.png");  // Different ship for tracker
    _context.window->loadTexture(ENEMY_ZIGZAG_KEY, "assets/spaceship/Ship4.png");   // Different ship for zigzag
    _context.window->loadTexture(ENEMY_FAST_KEY, "assets/spaceship/Ship5.png");     // Different ship for fast
    _context.window->loadTexture(ENEMY_BOMBER_KEY, "assets/spaceship/Ship6.png");   // Different ship for bomber
    _context.window->loadTexture(ENEMY_POW_ARMOR_KEY, "assets/spaceship/Ship1.png"); // POWArmor (reuse Ship1, distinct orange in accessibility mode)
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

        // Toggle chat with configurable key (default: T)
        auto& accessConfig = accessibility::AccessibilityConfig::getInstance();
        if (accessConfig.isActionKey(accessibility::GameAction::OpenChat, key.key) &&
            !_context.udpClient->isLocalPlayerDead() && !_wasKicked) {
            _chatInputOpen = true;
            _skipNextTextEntered = true;  // Prevent key from appearing in chat input
            if (_chatInput) {
                _chatInput->setFocused(true);
            }
            return;
        }

        // Toggle chat expansion with configurable key (default: O)
        if (accessConfig.isActionKey(accessibility::GameAction::ExpandChat, key.key) &&
            !_chatDisplayMessages.empty()) {
            _chatExpanded = !_chatExpanded;
            return;
        }

        // Push-to-Talk (configurable key)
        if (accessConfig.isActionKey(accessibility::GameAction::PushToTalk, key.key) && !_chatInputOpen) {
            auto& voiceMgr = audio::VoiceChatManager::getInstance();
            if (voiceMgr.isConnected() &&
                voiceMgr.getVoiceMode() == audio::VoiceChatManager::VoiceMode::PushToTalk) {
                voiceMgr.startTalking();
            }
        }

        // R-Type Authentic (Phase 3): Force Pod toggle (configurable key, default: F)
        if (accessConfig.isActionKey(accessibility::GameAction::ForceToggle, key.key) &&
            !_chatInputOpen && _context.udpClient) {
            _context.udpClient->toggleForce();
            client::logging::Logger::getSceneLogger()->debug("Force Toggle requested");
        }

        // Toggle Controls HUD visibility (configurable key, default: H)
        if (accessConfig.isActionKey(accessibility::GameAction::ToggleControls, key.key) && !_chatInputOpen) {
            _showControlsHUD = !_showControlsHUD;
            return;
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

    // Update power-up positions (client-side drift simulation)
    _context.udpClient->updatePowerUps(adjustedDeltaTime);

    if (_context.udpClient->isLocalPlayerDead()) {
        // Cut PTT if player dies while holding the talk key
        auto& voiceMgr = audio::VoiceChatManager::getInstance();
        if (voiceMgr.isConnected() && voiceMgr.isTalking()) {
            voiceMgr.stopTalking();
        }
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
        // Weapon switching (configurable keys, default: Q = previous, E = next)
        if (accessConfig.isActionKey(accessibility::GameAction::WeaponPrev, key)) {
            inputKeys |= InputKeys::WEAPON_PREV;
        }
        if (accessConfig.isActionKey(accessibility::GameAction::WeaponNext, key)) {
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

    // R-Type Authentic (Phase 3): Wave Cannon charge system
    // Hold fire to charge, release to fire charged beam OR tap for normal shot
    // AUTO-FIRE: Before charge starts (< CHARGE_TIME_LV1), holding fire = continuous shooting
    if (shootPressed) {
        if (!_isCharging) {
            // Start charging and fire immediately if cooldown allows
            _isCharging = true;
            _chargeTimer = 0.0f;
            _clientChargeLevel = 0;
            _context.udpClient->startCharging();

            // Fire immediately on first press
            if (_shootCooldown <= 0.0f) {
                _context.udpClient->shootMissile();
                _shootCooldown = SHOOT_COOLDOWN_TIME;
                audio::AudioManager::getInstance().playSound("shoot");
            }
        } else {
            // Continue charging
            _chargeTimer += adjustedDeltaTime;

            // AUTO-FIRE: While holding and before charge level 1, keep firing (rapid fire mode)
            // This allows continuous shooting with weapons like Laser when holding the button
            if (_chargeTimer < WaveCannon::CHARGE_TIME_LV1 && _shootCooldown <= 0.0f) {
                _context.udpClient->shootMissile();
                _shootCooldown = SHOOT_COOLDOWN_TIME;
                audio::AudioManager::getInstance().playSound("shoot");
            }

            // Update charge level based on time (client-side prediction for UI)
            if (_chargeTimer >= WaveCannon::CHARGE_TIME_LV3) {
                _clientChargeLevel = 3;
            } else if (_chargeTimer >= WaveCannon::CHARGE_TIME_LV2) {
                _clientChargeLevel = 2;
            } else if (_chargeTimer >= WaveCannon::CHARGE_TIME_LV1) {
                _clientChargeLevel = 1;
            }
        }
    } else {
        // Fire button released
        if (_isCharging) {
            if (_clientChargeLevel > 0) {
                // Charged shot = Wave Cannon (only if we actually charged)
                _context.udpClient->releaseCharge();
                _shootCooldown = SHOOT_COOLDOWN_TIME * 2.0f;  // Longer cooldown for Wave Cannon
                audio::AudioManager::getInstance().playSound("shoot");  // TODO: different sound for Wave Cannon
                client::logging::Logger::getSceneLogger()->debug("Wave Cannon fired! Level: {}", _clientChargeLevel);
            }
            // Note: Normal shots are now handled in the holding phase (auto-fire)
            _isCharging = false;
            _chargeTimer = 0.0f;
            _clientChargeLevel = 0;
        }
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

void GameScene::renderTeamScoreboard()
{
    auto localId = _context.udpClient->getLocalPlayerId();
    auto players = _context.udpClient->getPlayers();

    // Only show if more than 1 player
    if (players.size() <= 1) return;

    // Sort players by score (descending)
    std::vector<client::network::NetworkPlayer> sortedPlayers(players.begin(), players.end());
    std::sort(sortedPlayers.begin(), sortedPlayers.end(),
              [](const auto& a, const auto& b) { return a.score > b.score; });

    // Position: right side, below the score HUD (moved left to avoid overflow)
    float boardWidth = 210.0f;
    float boardX = SCREEN_WIDTH - boardWidth - 15.0f;  // 15px margin from right edge
    float boardY = 110.0f;  // Below score HUD (which ends ~95px)
    float rowHeight = 22.0f;
    float boardHeight = 28.0f + sortedPlayers.size() * rowHeight;

    // Background
    _context.window->drawRect(boardX - 5.0f, boardY - 5.0f, boardWidth, boardHeight, {20, 20, 40, 200});

    // Header
    _context.window->drawText(FONT_KEY, "TEAM SCORES", boardX, boardY, 12, {200, 200, 200, 255});

    // Player rows
    float rowY = boardY + 20.0f;
    int rank = 1;
    for (const auto& player : sortedPlayers) {
        bool isLocal = localId && player.id == *localId;

        // Rank color (gold, silver, bronze, white)
        rgba rankColor;
        if (rank == 1) {
            rankColor = {255, 215, 0, 255};   // Gold
        } else if (rank == 2) {
            rankColor = {192, 192, 192, 255}; // Silver
        } else if (rank == 3) {
            rankColor = {205, 127, 50, 255};  // Bronze
        } else {
            rankColor = {150, 150, 150, 255}; // Gray
        }

        // Get player name from stored names map, fallback to "P#"
        std::string playerName;
        if (isLocal) {
            playerName = "YOU";
        } else {
            auto it = _playerNames.find(player.id);
            if (it != _playerNames.end() && !it->second.empty()) {
                // Truncate long names to 10 chars
                playerName = it->second.substr(0, 10);
            } else {
                playerName = "P" + std::to_string(player.id);
            }
        }
        rgba nameColor = isLocal ? rgba{100, 255, 100, 255} : rgba{255, 255, 255, 255};

        // Rank
        std::string rankStr = std::to_string(rank) + ".";
        _context.window->drawText(FONT_KEY, rankStr, boardX, rowY, 11, rankColor);

        // Name
        _context.window->drawText(FONT_KEY, playerName, boardX + 18.0f, rowY, 11, nameColor);

        // Score (right-aligned area)
        std::string scoreStr = std::to_string(player.score);
        _context.window->drawText(FONT_KEY, scoreStr, boardX + 95.0f, rowY, 11, {255, 255, 255, 255});

        // Combo multiplier (only if > 1.0x)
        if (player.combo > 10) {
            float comboVal = static_cast<float>(player.combo) / 10.0f;
            char comboBuf[16];
            std::snprintf(comboBuf, sizeof(comboBuf), "x%.1f", comboVal);

            // Color based on combo
            rgba comboColor;
            if (player.combo >= 25) {
                comboColor = {255, 100, 100, 255};  // Red (2.5x+)
            } else if (player.combo >= 20) {
                comboColor = {255, 200, 50, 255};   // Gold (2.0x+)
            } else {
                comboColor = {100, 255, 100, 255};  // Green
            }
            _context.window->drawText(FONT_KEY, comboBuf, boardX + 160.0f, rowY, 11, comboColor);
        }

        rowY += rowHeight;
        ++rank;
    }
}

void GameScene::renderWeaponHUD()
{
    auto localId = _context.udpClient->getLocalPlayerId();
    if (!localId) return;

    auto players = _context.udpClient->getPlayers();

    // Find local player's weapon and level
    uint8_t currentWeapon = 0;
    uint8_t weaponLevel = 0;
    for (const auto& p : players) {
        if (p.id == *localId) {
            currentWeapon = p.currentWeapon;
            weaponLevel = p.weaponLevel;
            break;
        }
    }

    // Weapon names (short versions to avoid text overflow)
    static const char* weaponNames[] = {"STD", "SPREAD", "LASER", "HOMING"};
    static const rgba weaponColors[] = {
        {180, 180, 180, 255},  // Standard - gray
        {100, 200, 255, 255},  // Spread - cyan
        {255, 100, 100, 255},  // Laser - red
        {100, 255, 100, 255}   // Missile/Homing - green
    };

    // Level indicator colors (brighter for higher levels)
    static const rgba levelColors[] = {
        {120, 120, 120, 255},  // Lv.0 - dim gray
        {180, 180, 100, 255},  // Lv.1 - yellow-ish
        {255, 180, 50, 255},   // Lv.2 - orange
        {255, 100, 255, 255}   // Lv.3 - magenta (MAX)
    };

    // Clamp weapon index and level
    if (currentWeapon >= 4) currentWeapon = 0;
    if (weaponLevel > 3) weaponLevel = 3;

    // Display position (top-left, below health bar to avoid chat overlap)
    float hudX = 20.0f;
    float hudY = 80.0f;  // Moved to top-left, below health HUD

    // Get configured keys for weapon switching
    auto& accessConfig = accessibility::AccessibilityConfig::getInstance();
    std::string prevKey = accessibility::AccessibilityConfig::keyToString(
        accessConfig.getPrimaryKey(accessibility::GameAction::WeaponPrev));
    std::string nextKey = accessibility::AccessibilityConfig::keyToString(
        accessConfig.getPrimaryKey(accessibility::GameAction::WeaponNext));
    std::string hint = "[" + prevKey + "/" + nextKey + "]";

    // Background (sized to fit content with level indicator)
    _context.window->drawRect(hudX - 5.0f, hudY - 5.0f, 110.0f, 55.0f, {20, 20, 40, 200});

    // Label with hint on same line
    std::string label = "WEAPON " + hint;
    _context.window->drawText(FONT_KEY, label, hudX, hudY, 10, {150, 150, 150, 255});

    // Current weapon name with color (larger, below label)
    _context.window->drawText(FONT_KEY, weaponNames[currentWeapon], hudX, hudY + 16.0f, 18, weaponColors[currentWeapon]);

    // Weapon level indicator (visual upgrade level)
    // Show level boxes: [■][■][□] for level 2
    float levelY = hudY + 38.0f;
    for (int i = 0; i < 3; ++i) {
        float boxX = hudX + i * 18.0f;
        if (i < weaponLevel) {
            // Filled box (upgraded)
            _context.window->drawRect(boxX, levelY, 14.0f, 10.0f, levelColors[weaponLevel]);
        } else {
            // Empty box (not yet upgraded)
            _context.window->drawRect(boxX, levelY, 14.0f, 10.0f, {50, 50, 60, 255});
        }
        // Border
        _context.window->drawRect(boxX, levelY, 14.0f, 2.0f, {80, 80, 100, 255});
        _context.window->drawRect(boxX, levelY + 8.0f, 14.0f, 2.0f, {80, 80, 100, 255});
    }

    // Level text next to boxes
    std::string levelText = weaponLevel > 0 ? "Lv." + std::to_string(weaponLevel) : "";
    if (weaponLevel == 3) levelText = "MAX";
    if (!levelText.empty()) {
        _context.window->drawText(FONT_KEY, levelText, hudX + 58.0f, levelY, 10, levelColors[weaponLevel]);
    }
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
    // Basic = 0, Tracker = 1, Zigzag = 2, Fast = 3, Bomber = 4, POWArmor = 5
    switch (enemyType) {
        case 0: return ENEMY_BASIC_KEY;      // Basic
        case 1: return ENEMY_TRACKER_KEY;    // Tracker
        case 2: return ENEMY_ZIGZAG_KEY;     // Zigzag
        case 3: return ENEMY_FAST_KEY;       // Fast
        case 4: return ENEMY_BOMBER_KEY;     // Bomber
        case 5: return ENEMY_POW_ARMOR_KEY;  // POWArmor (special power-up carrier)
        default: return ENEMY_BASIC_KEY;     // Fallback to basic
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
                case 5:  // POWArmor - orange (power-up carrier)
                    color = {255, 160, 50, 255};
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

    // Position: left side, below weapon HUD (which ends at ~140px)
    float indicatorX = 20.0f;
    float indicatorY = 145.0f;

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
    renderWaveCannons();   // Wave Cannon beams (Phase 3)
    renderPowerUps();      // Power-up items (Phase 3)
    renderForcePods();     // Force pods (Phase 3)
    renderBitDevices();    // Bit devices (Phase 3)
    renderEnemyMissiles();
    renderHUD();
    renderSpeedIndicator(); // Speed upgrade level (Phase 3)
    renderScoreHUD();
    renderTeamScoreboard(); // All players' scores in real-time (multiplayer)
    renderWeaponHUD();     // Weapon indicator (Gameplay Phase 2)
    renderChargeGauge();   // Wave Cannon charge gauge (Phase 3)
    renderBossHealthBar(); // Boss HP bar at top (Gameplay Phase 2)
    renderControlsHUD();   // Controls help (bottom-right)
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
// R-Type Authentic (Phase 3)
// ============================================================================

void GameScene::renderWaveCannons()
{
    if (!_context.udpClient || !_context.window) return;

    auto waveCannons = _context.udpClient->getWaveCannons();

    for (const auto& wc : waveCannons) {
        // Calculate beam height based on charge level
        float beamHeight;
        rgba beamColor;

        switch (wc.charge_level) {
            case 1:
                beamHeight = WaveCannon::WIDTH_LV1;
                beamColor = {100, 200, 255, 220};  // Light blue
                break;
            case 2:
                beamHeight = WaveCannon::WIDTH_LV2;
                beamColor = {200, 150, 255, 230};  // Purple-blue
                break;
            case 3:
            default:
                beamHeight = WaveCannon::WIDTH_LV3;
                beamColor = {255, 255, 100, 255};  // Bright yellow
                break;
        }

        // Draw the beam as a stretched rectangle from position to edge of screen
        float beamWidth = SCREEN_WIDTH - static_cast<float>(wc.x);
        float beamY = static_cast<float>(wc.y) - beamHeight / 2.0f;

        // Main beam
        _context.window->drawRect(
            static_cast<float>(wc.x),
            beamY,
            beamWidth,
            beamHeight,
            beamColor
        );

        // Inner glow (brighter core)
        float coreHeight = beamHeight * 0.4f;
        float coreY = static_cast<float>(wc.y) - coreHeight / 2.0f;
        _context.window->drawRect(
            static_cast<float>(wc.x),
            coreY,
            beamWidth,
            coreHeight,
            {255, 255, 255, 200}  // White core
        );
    }
}

void GameScene::renderPowerUps()
{
    if (!_context.udpClient || !_context.window) return;

    auto powerUps = _context.udpClient->getPowerUps();

    for (const auto& pu : powerUps) {
        // Choose color based on power-up type
        rgba puColor;
        std::string puLabel;

        // R-Type authentic power-up colors
        switch (static_cast<PowerUpType>(pu.type)) {
            case PowerUpType::Health:
                puColor = {255, 80, 80, 255};   // Red
                puLabel = "+HP";
                break;
            case PowerUpType::SpeedUp:
                puColor = {80, 80, 255, 255};   // Blue (classic R-Type blue crystal)
                puLabel = "SPD";
                break;
            case PowerUpType::WeaponCrystal:
                puColor = {255, 80, 80, 255};   // Red (classic R-Type red crystal)
                puLabel = "WPN";
                break;
            case PowerUpType::ForcePod:
                puColor = {255, 165, 0, 255};   // Orange (Force Pod orb)
                puLabel = "POD";
                break;
            case PowerUpType::BitDevice:
                puColor = {180, 100, 220, 255}; // Purple (Bit Device)
                puLabel = "BIT";
                break;
            default:
                puColor = {200, 200, 200, 255}; // Gray
                puLabel = "???";
                break;
        }

        float px = pu.x;
        float py = pu.y;

        // Draw pulsating effect (simple scaling based on remaining time)
        float pulse = 1.0f + 0.1f * std::sin(pu.remaining_time * 6.0f);
        float size = POWERUP_SIZE * pulse;

        // Outer glow
        _context.window->drawRect(
            px - size * 0.1f,
            py - size * 0.1f,
            size * 1.2f,
            size * 1.2f,
            {puColor.r, puColor.g, puColor.b, 80}
        );

        // Main power-up
        _context.window->drawRect(px, py, size, size, puColor);

        // Inner bright spot
        _context.window->drawRect(
            px + size * 0.25f,
            py + size * 0.25f,
            size * 0.5f,
            size * 0.5f,
            {255, 255, 255, 150}
        );

        // Label
        _context.window->drawText(FONT_KEY, puLabel,
            px + 2.0f, py - 14.0f, 10, {255, 255, 255, 200});
    }
}

void GameScene::renderForcePods()
{
    if (!_context.udpClient || !_context.window) return;

    auto forcePods = _context.udpClient->getForcePods();

    for (const auto& force : forcePods) {
        if (force.level == 0) continue;  // No Force

        float fx = static_cast<float>(force.x);
        float fy = static_cast<float>(force.y);

        // Color based on level
        rgba forceColor;
        if (force.level >= 2) {
            forceColor = {255, 200, 50, 255};   // Gold for level 2
        } else {
            forceColor = {100, 180, 255, 255};  // Blue for level 1
        }

        // Draw Force Pod as an ellipse-like shape (using rectangles)
        float width = FORCE_POD_WIDTH;
        float height = FORCE_POD_HEIGHT;

        // Outer glow
        _context.window->drawRect(
            fx - 4.0f, fy - 4.0f,
            width + 8.0f, height + 8.0f,
            {forceColor.r, forceColor.g, forceColor.b, 60}
        );

        // Main body
        _context.window->drawRect(fx, fy, width, height, forceColor);

        // Inner highlight
        _context.window->drawRect(
            fx + width * 0.2f, fy + height * 0.2f,
            width * 0.4f, height * 0.3f,
            {255, 255, 255, 180}
        );

        // Show "attached" indicator
        if (force.is_attached) {
            _context.window->drawText(FONT_KEY, "ATK",
                fx, fy - 12.0f, 8, {255, 255, 255, 180});
        }
    }
}

void GameScene::renderBitDevices()
{
    if (!_context.udpClient || !_context.window) return;

    auto bitDevices = _context.udpClient->getBitDevices();

    for (const auto& bit : bitDevices) {
        float bx = static_cast<float>(bit.x);
        float by = static_cast<float>(bit.y);

        // Purple color for Bit Devices (distinct from orange Force Pod)
        rgba bitColor = {180, 100, 220, 255};

        // Outer glow
        _context.window->drawRect(
            bx - 3.0f, by - 3.0f,
            BIT_DEVICE_SIZE + 6.0f, BIT_DEVICE_SIZE + 6.0f,
            {bitColor.r, bitColor.g, bitColor.b, 50}
        );

        // Main body
        _context.window->drawRect(bx, by, BIT_DEVICE_SIZE, BIT_DEVICE_SIZE, bitColor);

        // Inner highlight
        _context.window->drawRect(
            bx + BIT_DEVICE_SIZE * 0.3f, by + BIT_DEVICE_SIZE * 0.2f,
            BIT_DEVICE_SIZE * 0.3f, BIT_DEVICE_SIZE * 0.3f,
            {255, 200, 255, 200}
        );
    }
}

void GameScene::renderChargeGauge()
{
    if (!_context.udpClient || !_context.window) return;
    if (!_isCharging && _clientChargeLevel == 0) return;

    // Position gauge near the player
    auto localId = _context.udpClient->getLocalPlayerId();
    if (!localId) return;

    // Use predicted position for local player
    float playerX = _predictedX;
    float playerY = _predictedY;

    float gaugeX = playerX;
    float gaugeY = playerY + SHIP_HEIGHT + 8.0f;

    // Background
    _context.window->drawRect(
        gaugeX - 2.0f, gaugeY - 2.0f,
        CHARGE_GAUGE_WIDTH + 4.0f, CHARGE_GAUGE_HEIGHT + 4.0f,
        {20, 20, 40, 200}
    );

    // Calculate fill based on charge time
    float fillRatio = 0.0f;
    if (_chargeTimer > 0.0f) {
        fillRatio = std::min(_chargeTimer / WaveCannon::CHARGE_TIME_LV3, 1.0f);
    }
    float fillWidth = CHARGE_GAUGE_WIDTH * fillRatio;

    // Choose color based on charge level
    rgba gaugeColor;
    if (_clientChargeLevel >= 3) {
        gaugeColor = {255, 255, 100, 255};  // Yellow - max charge
    } else if (_clientChargeLevel >= 2) {
        gaugeColor = {200, 150, 255, 255};  // Purple
    } else if (_clientChargeLevel >= 1) {
        gaugeColor = {100, 200, 255, 255};  // Blue
    } else {
        gaugeColor = {100, 100, 150, 200};  // Gray - charging
    }

    // Fill bar
    if (fillWidth > 0) {
        _context.window->drawRect(gaugeX, gaugeY, fillWidth, CHARGE_GAUGE_HEIGHT, gaugeColor);
    }

    // Level markers
    float lv1Mark = CHARGE_GAUGE_WIDTH * (WaveCannon::CHARGE_TIME_LV1 / WaveCannon::CHARGE_TIME_LV3);
    float lv2Mark = CHARGE_GAUGE_WIDTH * (WaveCannon::CHARGE_TIME_LV2 / WaveCannon::CHARGE_TIME_LV3);

    _context.window->drawRect(gaugeX + lv1Mark, gaugeY, 2.0f, CHARGE_GAUGE_HEIGHT, {255, 255, 255, 100});
    _context.window->drawRect(gaugeX + lv2Mark, gaugeY, 2.0f, CHARGE_GAUGE_HEIGHT, {255, 255, 255, 100});

    // Level indicator text
    if (_clientChargeLevel > 0) {
        std::string levelText = "LV" + std::to_string(_clientChargeLevel);
        _context.window->drawText(FONT_KEY, levelText,
            gaugeX + CHARGE_GAUGE_WIDTH + 5.0f, gaugeY - 2.0f, 10, gaugeColor);
    }
}

void GameScene::renderSpeedIndicator()
{
    if (!_context.udpClient || !_context.window) return;

    auto localId = _context.udpClient->getLocalPlayerId();
    if (!localId) return;

    // Find local player's speed level
    auto players = _context.udpClient->getPlayers();
    uint8_t speedLevel = 0;

    for (const auto& p : players) {
        if (p.id == *localId) {
            speedLevel = p.speedLevel;
            break;
        }
    }

    // Position: below health bar
    constexpr float indicatorX = HUD_MARGIN;
    constexpr float indicatorY = HUD_MARGIN + HUD_HEALTH_BAR_HEIGHT + 8.0f;
    constexpr float barWidth = 15.0f;
    constexpr float barHeight = 10.0f;
    constexpr float barSpacing = 4.0f;
    constexpr uint8_t maxSpeedLevel = 3;

    // Background
    float totalWidth = 50.0f + maxSpeedLevel * (barWidth + barSpacing);
    _context.window->drawRect(indicatorX - 3.0f, indicatorY - 3.0f, totalWidth + 6.0f, barHeight + 6.0f, {20, 20, 40, 180});

    // "SPD" label
    _context.window->drawText(FONT_KEY, "SPD", indicatorX, indicatorY - 1.0f, 10, {150, 150, 150, 255});

    // Speed bars (3 max)
    float barX = indicatorX + 35.0f;
    for (uint8_t i = 0; i < maxSpeedLevel; ++i) {
        rgba barColor;
        if (i < speedLevel) {
            // Active bar - cyan gradient (brighter for higher levels)
            uint8_t intensity = static_cast<uint8_t>(150 + (i + 1) * 35);
            barColor = {0, intensity, intensity, 255};
        } else {
            // Inactive bar - dark gray
            barColor = {50, 50, 60, 200};
        }

        _context.window->drawRect(barX, indicatorY, barWidth, barHeight, barColor);
        barX += barWidth + barSpacing;
    }
}

void GameScene::renderControlsHUD()
{
    if (!_context.window) return;

    // Get configured keys
    auto& cfg = accessibility::AccessibilityConfig::getInstance();
    auto keyStr = [&cfg](accessibility::GameAction action) {
        return accessibility::AccessibilityConfig::keyToString(cfg.getPrimaryKey(action));
    };

    // Position: bottom-right corner with proper margins
    // Note: UBUNTU_OFFSET accounts for Ubuntu taskbar/dock cutting off bottom
    constexpr float hudWidth = 230.0f;
    constexpr float hudHeight = 195.0f;  // 6 lines × 22px + title(30) + padding(33)
    constexpr float margin = 15.0f;
    constexpr float UBUNTU_OFFSET = 30.0f;
    float hudX = SCREEN_WIDTH - hudWidth - margin;
    float hudY = SCREEN_HEIGHT - hudHeight - margin - UBUNTU_OFFSET;

    // If hidden, show only a small hint to toggle
    if (!_showControlsHUD) {
        std::string toggleHint = "[" + keyStr(accessibility::GameAction::ToggleControls) + "] Show Help";
        float hintWidth = 130.0f;
        _context.window->drawRect(SCREEN_WIDTH - hintWidth - margin, SCREEN_HEIGHT - 30.0f - UBUNTU_OFFSET, hintWidth, 22.0f, {10, 10, 30, 150});
        _context.window->drawText(FONT_KEY, toggleHint, SCREEN_WIDTH - hintWidth - margin + 10.0f, SCREEN_HEIGHT - 27.0f - UBUNTU_OFFSET, 10, {100, 100, 120, 255});
        return;
    }

    // Semi-transparent background
    _context.window->drawRect(hudX, hudY, hudWidth, hudHeight, {10, 10, 30, 180});

    // Title with hide hint
    std::string hideHint = "[" + keyStr(accessibility::GameAction::ToggleControls) + "] Hide";
    _context.window->drawText(FONT_KEY, "CONTROLS", hudX + 10.0f, hudY + 8.0f, 13, {255, 220, 100, 255});
    _context.window->drawText(FONT_KEY, hideHint, hudX + 110.0f, hudY + 10.0f, 9, {100, 100, 120, 255});

    // Control lines with colors
    float lineY = hudY + 30.0f;
    constexpr float lineSpacing = 22.0f;
    constexpr float keyX = 10.0f;    // Key column offset from hudX
    constexpr float descX = 70.0f;   // Description column offset from hudX
    rgba keyColor = {100, 200, 255, 255};   // Cyan for keys
    rgba descColor = {180, 180, 180, 255};  // Gray for descriptions

    // Movement (show Up/Left keys)
    std::string moveKeys = keyStr(accessibility::GameAction::MoveUp) + "/" +
                           keyStr(accessibility::GameAction::MoveLeft);
    _context.window->drawText(FONT_KEY, moveKeys, hudX + keyX, lineY, 10, keyColor);
    _context.window->drawText(FONT_KEY, "Move", hudX + descX, lineY, 10, descColor);
    lineY += lineSpacing;

    // Shoot / Wave Cannon
    _context.window->drawText(FONT_KEY, keyStr(accessibility::GameAction::Shoot), hudX + keyX, lineY, 10, keyColor);
    _context.window->drawText(FONT_KEY, "Shoot (hold=charge)", hudX + descX, lineY, 10, descColor);
    lineY += lineSpacing;

    // Force Pod
    _context.window->drawText(FONT_KEY, keyStr(accessibility::GameAction::ForceToggle), hudX + keyX, lineY, 10, keyColor);
    _context.window->drawText(FONT_KEY, "Toggle Force", hudX + descX, lineY, 10, descColor);
    lineY += lineSpacing;

    // Weapon Switch
    std::string weaponKeys = keyStr(accessibility::GameAction::WeaponPrev) + "/" +
                             keyStr(accessibility::GameAction::WeaponNext);
    _context.window->drawText(FONT_KEY, weaponKeys, hudX + keyX, lineY, 10, keyColor);
    _context.window->drawText(FONT_KEY, "Switch Weapon", hudX + descX, lineY, 10, descColor);
    lineY += lineSpacing;

    // Chat
    _context.window->drawText(FONT_KEY, keyStr(accessibility::GameAction::OpenChat), hudX + keyX, lineY, 10, keyColor);
    _context.window->drawText(FONT_KEY, "Open Chat", hudX + descX, lineY, 10, descColor);
    lineY += lineSpacing;

    // Voice
    _context.window->drawText(FONT_KEY, keyStr(accessibility::GameAction::PushToTalk), hudX + keyX, lineY, 10, keyColor);
    _context.window->drawText(FONT_KEY, "Push-to-Talk", hudX + descX, lineY, 10, descColor);
}

// ============================================================================
// Chat System (Phase 2)
// ============================================================================

void GameScene::initChatUI()
{
    if (_chatUIInitialized || !_context.window) return;

    // Chat input field - positioned in bottom-left chat area
    // Position: after "Say:" label at x=60, inside the chat overlay box
    // Note: UBUNTU_OFFSET (70px) accounts for Ubuntu taskbar/dock
    constexpr float UBUNTU_OFFSET = 70.0f;
    _chatInput = std::make_unique<ui::TextInput>(
        Vec2f{60.0f, SCREEN_HEIGHT - 52.0f - UBUNTU_OFFSET},
        Vec2f{440.0f, 28.0f},
        "Type your message...",
        FONT_KEY
    );
    _chatInput->setMaxLength(200);
    _chatInput->setBgColor({20, 20, 30, 200});
    _chatInput->setFocusedBgColor({30, 30, 45, 220});
    _chatInput->setBorderColor({60, 80, 120, 200});
    _chatInput->setFocusedBorderColor({80, 140, 220, 255});
    _chatInput->setTextColor({255, 255, 255, 255});
    _chatInput->setPlaceholderColor({100, 100, 120, 255});

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
        // Close chat on empty message (Enter without text = close and resume game)
        _chatInputOpen = false;
        return;
    }

    if (_context.tcpClient && _context.tcpClient->isConnected()) {
        _context.tcpClient->sendChatMessage(message);
    }

    _chatInput->clear();

    // Check setting: keep chat open for consecutive messages or close to resume playing
    auto& accessConfig = accessibility::AccessibilityConfig::getInstance();
    if (!accessConfig.getKeepChatOpenAfterSend()) {
        _chatInputOpen = false;
    }
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

    // Get configured keys for chat controls
    auto& accessConfig = accessibility::AccessibilityConfig::getInstance();
    std::string expandKey = accessibility::AccessibilityConfig::keyToString(
        accessConfig.getPrimaryKey(accessibility::GameAction::ExpandChat));
    std::string chatKey = accessibility::AccessibilityConfig::keyToString(
        accessConfig.getPrimaryKey(accessibility::GameAction::OpenChat));

    // Constants for chat display
    constexpr float msgX = 20.0f;
    constexpr float msgSpacing = 26.0f;
    constexpr size_t RECENT_MSG_COUNT = 3;  // Show only 3 recent messages when not expanded

    // Calculate base Y position (above chat input area)
    // Note: Offset by 70px to account for Ubuntu taskbar/dock cutting off bottom of window
    // TODO: Remove this workaround when fullscreen/resize is implemented
    constexpr float UBUNTU_OFFSET = 70.0f;
    float baseY = _chatInputOpen ? SCREEN_HEIGHT - 120.0f - UBUNTU_OFFSET : SCREEN_HEIGHT - 70.0f - UBUNTU_OFFSET;

    // Determine which messages to show
    // _chatExpanded = false: show only last 3 messages (recent + non-expired)
    // _chatExpanded = true: show ALL messages
    std::vector<size_t> messagesToShow;

    if (_chatExpanded) {
        // Show all messages
        for (size_t i = 0; i < _chatDisplayMessages.size(); ++i) {
            messagesToShow.push_back(i);
        }
    } else {
        // Show only last RECENT_MSG_COUNT messages (regardless of expired status)
        size_t startIdx = _chatDisplayMessages.size() > RECENT_MSG_COUNT
            ? _chatDisplayMessages.size() - RECENT_MSG_COUNT : 0;
        for (size_t i = startIdx; i < _chatDisplayMessages.size(); ++i) {
            messagesToShow.push_back(i);
        }
    }

    // Draw messages from bottom to top (most recent at bottom)
    float msgY = baseY;
    for (auto it = messagesToShow.rbegin(); it != messagesToShow.rend(); ++it) {
        const auto& msg = _chatDisplayMessages[*it];

        // Calculate alpha based on remaining display time (fade out effect)
        // When expanded, always show at full opacity
        uint8_t alpha = 255;
        if (!_chatExpanded && !msg.expired && msg.displayTime < 2.0f) {
            alpha = static_cast<uint8_t>(255 * (msg.displayTime / 2.0f));
        }
        // Expired messages shown at reduced opacity when not expanded
        if (!_chatExpanded && msg.expired) {
            alpha = 150;
        }

        // Semi-transparent background for readability
        float textWidth = 420.0f;
        _context.window->drawRect(msgX - 5, msgY - 2, textWidth, 24, {0, 0, 0, static_cast<uint8_t>(alpha * 0.5f)});

        // Draw name in cyan/blue
        std::string nameText = msg.displayName + ": ";
        _context.window->drawText(FONT_KEY, nameText, msgX, msgY, 14, {100, 180, 255, alpha});

        // Draw message in white (truncate if too long)
        std::string displayText = msg.message;
        if (displayText.length() > 45) {
            displayText = displayText.substr(0, 42) + "...";
        }
        _context.window->drawText(FONT_KEY, displayText, msgX + 100, msgY, 14, {255, 255, 255, alpha});

        msgY -= msgSpacing;
    }

    // Show expand/collapse hint if there are more messages than shown
    size_t totalMessages = _chatDisplayMessages.size();
    if (totalMessages > RECENT_MSG_COUNT && !_chatInputOpen) {
        std::string expandHint;
        if (_chatExpanded) {
            expandHint = "[" + expandKey + "] Hide old messages";
        } else {
            size_t hiddenCount = totalMessages - RECENT_MSG_COUNT;
            expandHint = "[" + expandKey + "] Show " + std::to_string(hiddenCount) + " more";
        }
        _context.window->drawRect(msgX - 5, msgY - 2, 200, 20, {40, 40, 60, 160});
        _context.window->drawText(FONT_KEY, expandHint, msgX, msgY, 11, {150, 150, 180, 255});
    }

    // Render chat input if open
    if (_chatInputOpen && _chatInput) {
        // Dark background for input area
        _context.window->drawRect(15, SCREEN_HEIGHT - 60 - UBUNTU_OFFSET, 500, 55, {0, 0, 0, 220});
        // Top border (accent)
        _context.window->drawRect(15, SCREEN_HEIGHT - 60 - UBUNTU_OFFSET, 500, 2, {60, 120, 180, 255});

        // "Say:" label
        _context.window->drawText(FONT_KEY, "Say:", 25, SCREEN_HEIGHT - 45 - UBUNTU_OFFSET, 14, {100, 180, 255, 255});

        // Render the text input (shows what user is typing)
        _chatInput->render(*_context.window);

        // Hint text at bottom - adapts based on keepChatOpen setting
        std::string chatHintText;
        if (accessConfig.getKeepChatOpenAfterSend()) {
            chatHintText = "[Enter] Send  [Esc] Close & Resume";
        } else {
            chatHintText = "[Enter] Send & Resume  [Esc] Cancel";
        }
        _context.window->drawText(FONT_KEY, chatHintText,
            25, SCREEN_HEIGHT - 18 - UBUNTU_OFFSET, 11, {120, 120, 140, 255});
    } else {
        // Always show chat hint when not typing (bottom-left corner)
        std::string chatHint = "[" + chatKey + "] Chat";
        if (totalMessages > RECENT_MSG_COUNT) {
            chatHint += "  [" + expandKey + "] History";
        }
        _context.window->drawRect(msgX - 5, SCREEN_HEIGHT - 32 - UBUNTU_OFFSET, 180, 22, {20, 20, 30, 180});
        _context.window->drawText(FONT_KEY, chatHint, msgX, SCREEN_HEIGHT - 28 - UBUNTU_OFFSET, 12, {120, 150, 180, 255});
    }
}
