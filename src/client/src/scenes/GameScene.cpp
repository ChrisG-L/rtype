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
#include <variant>
#include <algorithm>
#include <cmath>

GameScene::GameScene()
{
    client::logging::Logger::getSceneLogger()->debug("GameScene created");
}

void GameScene::loadAssets()
{
    if (_assetsLoaded || !_context.window) return;

    _context.window->loadTexture(SHIP_TEXTURE_KEY, "assets/spaceship/Ship1.png");
    _context.window->loadTexture(MISSILE_TEXTURE_KEY, "assets/spaceship/missile.png");
    _context.window->loadTexture(ENEMY_TEXTURE_KEY, "assets/spaceship/Ship1.png");
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

void GameScene::handleEvent(const events::Event& event)
{
    if (std::holds_alternative<events::KeyPressed>(event)) {
        auto& key = std::get<events::KeyPressed>(event);
        _keysPressed.insert(key.key);

        // If kicked, any key returns to main menu
        if (_wasKicked && _sceneManager) {
            _sceneManager->changeScene(std::make_unique<MainMenuScene>());
            return;
        }
    } else if (std::holds_alternative<events::KeyReleased>(event)) {
        auto& key = std::get<events::KeyReleased>(event);
        _keysPressed.erase(key.key);
    }
}

void GameScene::processUDPEvents()
{
    if (!_context.udpClient) return;

    while (auto eventOpt = _context.udpClient->pollEvent()) {
        std::visit([this](auto&& event) {
            using T = std::decay_t<decltype(event)>;

            if constexpr (std::is_same_v<T, client::network::UDPKickedEvent>) {
                client::logging::Logger::getSceneLogger()->warn("Kicked from game!");
                _wasKicked = true;
            }
            // Other events are handled by UDPClient internally
        }, *eventOpt);
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

    // Process UDP events (check for kick)
    processUDPEvents();

    // If kicked, don't update gameplay
    if (_wasKicked) {
        return;
    }

    auto& accessConfig = accessibility::AccessibilityConfig::getInstance();
    float adjustedDeltaTime = deltatime * accessConfig.getGameSpeedMultiplier();

    for (auto& star : _stars) {
        star.x -= star.speed * adjustedDeltaTime;
        if (star.x < 0) {
            star.x = SCREEN_WIDTH;
        }
    }

    if (_context.udpClient->isLocalPlayerDead()) {
        return;
    }

    float dx = 0.0f;
    float dy = 0.0f;

    for (const auto& key : _keysPressed) {
        if (accessConfig.isActionKey(accessibility::GameAction::MoveUp, key)) {
            dy -= MOVE_SPEED * adjustedDeltaTime;
        }
        if (accessConfig.isActionKey(accessibility::GameAction::MoveDown, key)) {
            dy += MOVE_SPEED * adjustedDeltaTime;
        }
        if (accessConfig.isActionKey(accessibility::GameAction::MoveLeft, key)) {
            dx -= MOVE_SPEED * adjustedDeltaTime;
        }
        if (accessConfig.isActionKey(accessibility::GameAction::MoveRight, key)) {
            dx += MOVE_SPEED * adjustedDeltaTime;
        }
    }

    if (dx != 0.0f || dy != 0.0f) {
        int newX = static_cast<int>(_localX) + static_cast<int>(dx);
        int newY = static_cast<int>(_localY) + static_cast<int>(dy);

        newX = std::clamp(newX, 0, 1920 - static_cast<int>(SHIP_WIDTH));
        newY = std::clamp(newY, 0, 1080 - static_cast<int>(SHIP_HEIGHT));

        _localX = static_cast<uint16_t>(newX);
        _localY = static_cast<uint16_t>(newY);

        _context.udpClient->movePlayer(_localX, _localY);
    }

    if (_shootCooldown > 0.0f) {
        _shootCooldown -= adjustedDeltaTime;
    }

    bool shootPressed = false;
    for (const auto& key : _keysPressed) {
        if (accessConfig.isActionKey(accessibility::GameAction::Shoot, key)) {
            shootPressed = true;
            break;
        }
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

void GameScene::renderPlayers()
{
    auto localId = _context.udpClient->getLocalPlayerId();
    auto players = _context.udpClient->getPlayers();

    for (const auto& player : players) {
        if (!player.alive) {
            continue;
        }

        float px = static_cast<float>(player.x);
        float py = static_cast<float>(player.y);

        if (_assetsLoaded) {
            _context.window->drawSprite(
                SHIP_TEXTURE_KEY,
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

void GameScene::renderEnemies()
{
    auto enemies = _context.udpClient->getEnemies();

    for (const auto& enemy : enemies) {
        if (_assetsLoaded) {
            _context.window->drawSprite(
                ENEMY_TEXTURE_KEY,
                static_cast<float>(enemy.x),
                static_cast<float>(enemy.y),
                ENEMY_WIDTH,
                ENEMY_HEIGHT
            );
        } else {
            auto& accessConfig = accessibility::AccessibilityConfig::getInstance();
            auto c = accessConfig.getEnemyColor();
            _context.window->drawRect(
                static_cast<float>(enemy.x),
                static_cast<float>(enemy.y),
                ENEMY_WIDTH,
                ENEMY_HEIGHT,
                {c.r, c.g, c.b, c.a}
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
    _context.window->drawRect(0, 0, SCREEN_WIDTH + 20, SCREEN_HEIGHT + 20, {0, 0, 0, 180});

    const std::string gameOverText = "GAME OVER";
    float textX = SCREEN_WIDTH / 2.0f - 100.0f;
    float textY = SCREEN_HEIGHT / 2.0f - 50.0f;

    if (_assetsLoaded) {
        _context.window->drawText(FONT_KEY, gameOverText, textX, textY, 48, {255, 50, 50, 255});

        const std::string instructionText = "You have been eliminated";
        float instrX = SCREEN_WIDTH / 2.0f - 120.0f;
        float instrY = textY + 80.0f;
        _context.window->drawText(FONT_KEY, instructionText, instrX, instrY, 24, {200, 200, 200, 255});
    } else {
        _context.window->drawRect(textX, textY, 200.0f, 60.0f, {255, 50, 50, 255});
    }
}

void GameScene::renderKickedScreen()
{
    _context.window->drawRect(0, 0, SCREEN_WIDTH + 20, SCREEN_HEIGHT + 20, {0, 0, 0, 200});

    const std::string kickedText = "KICKED";
    float textX = SCREEN_WIDTH / 2.0f - 80.0f;
    float textY = SCREEN_HEIGHT / 2.0f - 50.0f;

    if (_assetsLoaded) {
        _context.window->drawText(FONT_KEY, kickedText, textX, textY, 48, {255, 150, 50, 255});

        const std::string instructionText = "You have been kicked from the game";
        float instrX = SCREEN_WIDTH / 2.0f - 180.0f;
        float instrY = textY + 80.0f;
        _context.window->drawText(FONT_KEY, instructionText, instrX, instrY, 24, {200, 200, 200, 255});

        const std::string hintText = "Press any key to return to menu";
        float hintX = SCREEN_WIDTH / 2.0f - 150.0f;
        float hintY = textY + 130.0f;
        _context.window->drawText(FONT_KEY, hintText, hintX, hintY, 18, {150, 150, 150, 255});
    } else {
        _context.window->drawRect(textX, textY, 160.0f, 60.0f, {255, 150, 50, 255});
    }
}

void GameScene::render()
{
    if (!_context.window || !_context.udpClient) return;

    renderBackground();
    renderEnemies();
    renderPlayers();
    renderMissiles();
    renderEnemyMissiles();
    renderHUD();

    if (_wasKicked) {
        renderKickedScreen();
    } else if (_context.udpClient->isLocalPlayerDead()) {
        renderDeathScreen();
    }
}
