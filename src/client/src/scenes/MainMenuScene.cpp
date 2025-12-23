/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** MainMenuScene
*/

#include "scenes/MainMenuScene.hpp"
#include "scenes/SceneManager.hpp"
#include "scenes/GameScene.hpp"
#include <variant>
#include <random>

MainMenuScene::MainMenuScene()
{
}

void MainMenuScene::loadAssets()
{
    if (_assetsLoaded || !_context.window) return;

    _context.window->loadFont(FONT_KEY, "assets/fonts/ARIA.TTF");
    _assetsLoaded = true;
}

void MainMenuScene::initStars()
{
    if (_starsInitialized) return;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> xDist(0, SCREEN_WIDTH);
    std::uniform_real_distribution<float> yDist(0, SCREEN_HEIGHT);
    std::uniform_real_distribution<float> speedDist(20.0f, 100.0f);
    std::uniform_real_distribution<float> sizeDist(1.0f, 3.0f);
    std::uniform_int_distribution<int> brightnessDist(100, 255);

    _stars.reserve(STAR_COUNT);
    for (int i = 0; i < STAR_COUNT; ++i) {
        _stars.push_back({
            xDist(gen),
            yDist(gen),
            speedDist(gen),
            sizeDist(gen),
            brightnessDist(gen)
        });
    }
    _starsInitialized = true;
}

void MainMenuScene::initUI()
{
    if (_uiInitialized || !_context.window) return;

    float centerX = SCREEN_WIDTH / 2.0f;
    float buttonWidth = 280.0f;
    float buttonHeight = 60.0f;
    float startY = 450.0f;
    float spacing = 80.0f;

    // Play button
    _playButton = std::make_unique<ui::Button>(
        Vec2f{centerX - buttonWidth / 2, startY},
        Vec2f{buttonWidth, buttonHeight},
        "PLAY",
        FONT_KEY
    );
    _playButton->setOnClick([this]() { onPlayClick(); });
    _playButton->setNormalColor({50, 100, 150, 255});
    _playButton->setHoveredColor({70, 130, 180, 255});

    // Settings button
    _settingsButton = std::make_unique<ui::Button>(
        Vec2f{centerX - buttonWidth / 2, startY + spacing},
        Vec2f{buttonWidth, buttonHeight},
        "SETTINGS",
        FONT_KEY
    );
    _settingsButton->setOnClick([this]() { onSettingsClick(); });

    // Quit button
    _quitButton = std::make_unique<ui::Button>(
        Vec2f{centerX - buttonWidth / 2, startY + spacing * 2},
        Vec2f{buttonWidth, buttonHeight},
        "QUIT",
        FONT_KEY
    );
    _quitButton->setOnClick([this]() { onQuitClick(); });
    _quitButton->setNormalColor({100, 50, 50, 255});
    _quitButton->setHoveredColor({150, 70, 70, 255});

    _uiInitialized = true;
}

void MainMenuScene::onPlayClick()
{
    // Connect to UDP game server
    if (_context.udpClient) {
        _context.udpClient->connect("127.0.0.1", 4124);
    }

    // Navigate to GameScene
    if (_sceneManager) {
        _sceneManager->changeScene(std::make_unique<GameScene>());
    }
}

void MainMenuScene::onSettingsClick()
{
    // TODO: Implement settings scene
}

void MainMenuScene::onQuitClick()
{
    if (_context.window) {
        _context.window->close();
    }
}

void MainMenuScene::handleEvent(const events::Event& event)
{
    if (!_uiInitialized) return;

    _playButton->handleEvent(event);
    _settingsButton->handleEvent(event);
    _quitButton->handleEvent(event);
}

void MainMenuScene::updateStars(float deltaTime)
{
    for (auto& star : _stars) {
        star.x -= star.speed * deltaTime;
        if (star.x < 0) {
            star.x = SCREEN_WIDTH;
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<float> yDist(0, SCREEN_HEIGHT);
            star.y = yDist(gen);
        }
    }
}

void MainMenuScene::update(float deltaTime)
{
    if (!_assetsLoaded) loadAssets();
    if (!_starsInitialized) initStars();
    if (!_uiInitialized) initUI();

    updateStars(deltaTime);

    _playButton->update(deltaTime);
    _settingsButton->update(deltaTime);
    _quitButton->update(deltaTime);
}

void MainMenuScene::render()
{
    if (!_context.window) return;

    // Draw space background
    _context.window->drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, {5, 5, 20, 255});

    // Draw stars
    for (const auto& star : _stars) {
        _context.window->drawRect(star.x, star.y, star.size, star.size,
            {star.brightness, star.brightness, star.brightness, 255});
    }

    // Draw title
    _context.window->drawText(FONT_KEY, "R-TYPE",
        SCREEN_WIDTH / 2 - 150, 180, 80, {100, 150, 255, 255});

    // Draw subtitle
    _context.window->drawText(FONT_KEY, "MAIN MENU",
        SCREEN_WIDTH / 2 - 80, 280, 32, {150, 150, 180, 255});

    // Draw buttons
    _playButton->render(*_context.window);
    _settingsButton->render(*_context.window);
    _quitButton->render(*_context.window);

    // Draw version
    _context.window->drawText(FONT_KEY, "v0.1.0",
        SCREEN_WIDTH - 100, SCREEN_HEIGHT - 40, 16, {80, 80, 100, 255});
}
