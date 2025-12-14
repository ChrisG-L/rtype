/*
** EPITECH PROJECT, 2025
** rtype [WSL : Ubuntu-24.04]
** File description:
** GameScene
*/

#include "scenes/GameScene.hpp"
#include "scenes/SceneManager.hpp"
#include "core/Logger.hpp"
#include "events/Event.hpp"
#include "utils/Vecs.hpp"
#include <variant>
#include <algorithm>

GameScene::GameScene()
{
    client::logging::Logger::getSceneLogger()->debug("GameScene created");
}

void GameScene::handleEvent(const events::Event& event)
{
    if (std::holds_alternative<events::KeyPressed>(event)) {
        auto& key = std::get<events::KeyPressed>(event);
        _keysPressed.insert(key.key);
    } else if (std::holds_alternative<events::KeyReleased>(event)) {
        auto& key = std::get<events::KeyReleased>(event);
        _keysPressed.erase(key.key);
    }
}

void GameScene::update(float deltatime)
{
    if (!_context.udpClient) return;

    float dx = 0.0f;
    float dy = 0.0f;

    if (_keysPressed.contains(events::Key::Up) || _keysPressed.contains(events::Key::Z)) {
        dy -= MOVE_SPEED * deltatime;
    }
    if (_keysPressed.contains(events::Key::Down) || _keysPressed.contains(events::Key::S)) {
        dy += MOVE_SPEED * deltatime;
    }
    if (_keysPressed.contains(events::Key::Left) || _keysPressed.contains(events::Key::Q)) {
        dx -= MOVE_SPEED * deltatime;
    }
    if (_keysPressed.contains(events::Key::Right) || _keysPressed.contains(events::Key::D)) {
        dx += MOVE_SPEED * deltatime;
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
}

void GameScene::render()
{
    if (!_context.window || !_context.udpClient) return;

    auto localId = _context.udpClient->getLocalPlayerId();
    auto players = _context.udpClient->getPlayers();

    for (const auto& player : players) {
        rgba color;
        if (localId && player.id == *localId) {
            color = {0, 100, 255, 255};
        } else {
            color = {0, 200, 100, 255};
        }

        _context.window->drawRect(
            static_cast<float>(player.x),
            static_cast<float>(player.y),
            SHIP_WIDTH,
            SHIP_HEIGHT,
            color
        );
    }
}
