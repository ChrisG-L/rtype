/*
** EPITECH PROJECT, 2025
** rtype [WSL : Ubuntu-24.04]
** File description:
** LoginScene
*/

#ifndef LOGINSCENE_HPP_
#define LOGINSCENE_HPP_

#include "graphics/Graphics.hpp"
#include "scenes/IScene.hpp"
#include "events/Event.hpp"
#include "graphics/IWindow.hpp"
#include "../core/IRenderer.hpp"
#include "gameplay/EntityManager.hpp"

#include <memory>
#include <unordered_set>

class LoginScene : public IScene
{
    public:
        LoginScene(std::shared_ptr<core::IRenderer>);
        ~LoginScene() override = default;

        void handleEvent(const events::Event& event) override;
        void update(float deltatime) override;
        void render() override;

    private:
        void handleInput(const events::Event& event);
        void spawnMissile();

        graphic::GraphicTexture* findTexture(const std::string& name);
        graphic::GraphicElement* findElement(const std::string& name);

        std::shared_ptr<core::IRenderer> _renderer;
        GraphicAssets _textures;
        GraphicAssetsE _elements;
        gameplay::EntityManager _entityManager;

        std::unordered_set<events::Key> _pressedKeys;
        float _moveSpeed = 600.0f;
        float _fireCooldown = 0.0f;
        std::size_t _entitiesIds = 0;
        static constexpr float FIRE_RATE = 0.15f;
};

#endif /* !LOGINSCENE_HPP_ */
