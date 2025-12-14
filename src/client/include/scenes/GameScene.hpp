/*
** EPITECH PROJECT, 2025
** rtype [WSL : Ubuntu-24.04]
** File description:
** GameScene
*/

#ifndef GAMESCENE_HPP_
#define GAMESCENE_HPP_

#include "scenes/IScene.hpp"
#include "events/Event.hpp"
#include "graphics/IWindow.hpp"
#include <iostream>
#include <unordered_set>

class GameScene : public IScene
{
public:
    GameScene();
    ~GameScene() override = default;

    void handleEvent(const events::Event& event) override;
    void update(float deltatime) override;
    void render() override;

private:
    uint16_t _localX = 100;
    uint16_t _localY = 300;
    std::unordered_set<events::Key> _keysPressed;
    static constexpr float MOVE_SPEED = 200.0f;
    static constexpr float SHIP_WIDTH = 50.0f;
    static constexpr float SHIP_HEIGHT = 30.0f;
};
#endif /* !GAMESCENE_HPP_ */
