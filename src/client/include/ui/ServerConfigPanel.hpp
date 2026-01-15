/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** ServerConfigPanel - Reusable server configuration UI component
*/

#ifndef SERVERCONFIGPANEL_HPP_
#define SERVERCONFIGPANEL_HPP_

#include "TextInput.hpp"
#include "Button.hpp"
#include "../graphics/IWindow.hpp"
#include "../events/Event.hpp"
#include "../utils/Vecs.hpp"
#include <memory>
#include <functional>
#include <string>

namespace ui {

/**
 * @brief Reusable panel for server configuration
 *
 * This component provides a complete UI for configuring server connection:
 * - Host address input
 * - TCP/UDP port inputs
 * - Quick preset buttons (FRANCE/LOCAL)
 * - Connect/Cancel buttons
 *
 * Usage:
 *   panel.setOnConnect([](){ ... });  // Called when user clicks CONNECT or preset
 *   panel.setOnCancel([](){ ... });   // Called when user clicks CANCEL
 */
class ServerConfigPanel {
public:
    using Callback = std::function<void()>;

    /**
     * @brief Construct panel centered on screen
     * @param screenWidth Screen width for centering
     * @param screenHeight Screen height for centering
     * @param fontKey Font key for text rendering
     */
    ServerConfigPanel(float screenWidth, float screenHeight, const std::string& fontKey);
    ~ServerConfigPanel() = default;

    // Non-copyable, movable
    ServerConfigPanel(const ServerConfigPanel&) = delete;
    ServerConfigPanel& operator=(const ServerConfigPanel&) = delete;
    ServerConfigPanel(ServerConfigPanel&&) = default;
    ServerConfigPanel& operator=(ServerConfigPanel&&) = default;

    /**
     * @brief Set callback for when user applies configuration
     * Called when CONNECT button or preset (FRANCE/LOCAL) is clicked.
     * Config is already saved to ServerConfigManager when this is called.
     */
    void setOnConnect(Callback callback) { _onConnect = std::move(callback); }

    /**
     * @brief Set callback for when user cancels
     * Called when CANCEL button is clicked or Escape is pressed.
     */
    void setOnCancel(Callback callback) { _onCancel = std::move(callback); }

    /**
     * @brief Refresh input fields from ServerConfigManager
     * Call this before showing the panel to display current config values.
     */
    void refreshFromConfig();

    /**
     * @brief Handle input events
     * @param event The event to handle
     * @return true if event was consumed by panel
     */
    bool handleEvent(const events::Event& event);

    /**
     * @brief Update panel animations
     * @param deltaTime Time since last update
     */
    void update(float deltaTime);

    /**
     * @brief Render the panel
     * @param window Window to render to
     */
    void render(graphics::IWindow& window);

private:
    void initUI();
    void applyConfig();
    void applyPreset(bool isFrance);

    float _screenWidth;
    float _screenHeight;
    std::string _fontKey;
    bool _initialized = false;

    // UI components
    std::unique_ptr<TextInput> _hostInput;
    std::unique_ptr<TextInput> _tcpPortInput;
    std::unique_ptr<TextInput> _udpPortInput;
    std::unique_ptr<Button> _franceButton;
    std::unique_ptr<Button> _localButton;
    std::unique_ptr<Button> _connectButton;
    std::unique_ptr<Button> _cancelButton;

    // Callbacks
    Callback _onConnect;
    Callback _onCancel;

    // Layout constants
    static constexpr float PANEL_WIDTH = 450.0f;
    static constexpr float PANEL_HEIGHT = 420.0f;
    static constexpr float CONTENT_MARGIN = 30.0f;
    static constexpr float INPUT_HEIGHT = 40.0f;
    static constexpr float LABEL_HEIGHT = 25.0f;
    static constexpr float BUTTON_HEIGHT = 45.0f;
};

} // namespace ui

#endif /* !SERVERCONFIGPANEL_HPP_ */
