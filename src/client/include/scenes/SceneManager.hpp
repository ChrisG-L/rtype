/*
** EPITECH PROJECT, 2025
** rtype [WSL : Ubuntu-24.04]
** File description:
** SceneManager
*/

#ifndef SCENEMANAGER_HPP_
#define SCENEMANAGER_HPP_

#include "scenes/IScene.hpp"
#include "../core/IRenderer.hpp"
#include <memory>
#include <stack>
#include <functional>

class SceneManager
{
    public:
        SceneManager();
        ~SceneManager();
        void setContext(const GameContext& ctx);

        // Replace entire scene stack with a single scene
        void changeScene(std::unique_ptr<IScene> newScene);

        // Push a scene on top (overlay) - current scene is paused
        void pushScene(std::unique_ptr<IScene> scene);

        // Pop the top scene - returns to previous scene
        void popScene();

        // Check if there's an overlay scene
        bool hasOverlay() const;

        // Get current scene count
        size_t sceneCount() const;

        void handleEvent(const events::Event& event);
        void update(float deltatime);
        void render();

        // Callback for disabling auto-reconnect (set by GameLoop)
        using DisableAutoReconnectCallback = std::function<void()>;
        void setDisableAutoReconnectCallback(DisableAutoReconnectCallback callback) {
            _disableAutoReconnectCallback = callback;
        }
        void disableAutoReconnect() {
            if (_disableAutoReconnectCallback) {
                _disableAutoReconnectCallback();
            }
        }

    private:
        enum class PendingAction {
            None,
            Change,
            Push,
            Pop
        };

        void applyPendingActions();
        void initScene(IScene* scene);
        IScene* currentScene() const;

        std::stack<std::unique_ptr<IScene>> _sceneStack;
        std::unique_ptr<IScene> _pendingScene;
        PendingAction _pendingAction = PendingAction::None;
        std::shared_ptr<core::IRenderer> _renderer;
        GameContext _context;
        DisableAutoReconnectCallback _disableAutoReconnectCallback;
};

#endif /* !SCENEMANAGER_HPP_ */
