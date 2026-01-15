/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** ServerConfigPanel - Reusable server configuration UI component
*/

#include "ui/ServerConfigPanel.hpp"
#include "config/ServerConfigManager.hpp"
#include <stdexcept>

namespace ui {

ServerConfigPanel::ServerConfigPanel(float screenWidth, float screenHeight, const std::string& fontKey)
    : _screenWidth(screenWidth)
    , _screenHeight(screenHeight)
    , _fontKey(fontKey)
{
    initUI();
}

void ServerConfigPanel::initUI()
{
    if (_initialized) return;

    auto& serverConfig = config::ServerConfigManager::getInstance();

    // Panel position (centered)
    float panelX = _screenWidth / 2 - PANEL_WIDTH / 2;
    float panelY = _screenHeight / 2 - 180.0f;

    // Layout inside panel
    float contentX = panelX + CONTENT_MARGIN;
    float inputWidth = PANEL_WIDTH - CONTENT_MARGIN * 2;
    float inputStartY = panelY + 70.0f;

    // Host input
    _hostInput = std::make_unique<TextInput>(
        Vec2f{contentX, inputStartY + LABEL_HEIGHT},
        Vec2f{inputWidth, INPUT_HEIGHT},
        "Adresse serveur",
        _fontKey
    );
    _hostInput->setText(serverConfig.getHost());
    _hostInput->setMaxLength(63);

    // Port inputs row
    float portsY = inputStartY + LABEL_HEIGHT + INPUT_HEIGHT + 40.0f;
    float portInputWidth = (inputWidth - 20.0f) / 2.0f;

    // TCP Port input
    _tcpPortInput = std::make_unique<TextInput>(
        Vec2f{contentX, portsY + LABEL_HEIGHT},
        Vec2f{portInputWidth, INPUT_HEIGHT},
        "Port TCP",
        _fontKey
    );
    _tcpPortInput->setText(std::to_string(serverConfig.getTcpPort()));
    _tcpPortInput->setMaxLength(5);

    // UDP Port input
    _udpPortInput = std::make_unique<TextInput>(
        Vec2f{contentX + portInputWidth + 20.0f, portsY + LABEL_HEIGHT},
        Vec2f{portInputWidth, INPUT_HEIGHT},
        "Port UDP",
        _fontKey
    );
    _udpPortInput->setText(std::to_string(serverConfig.getUdpPort()));
    _udpPortInput->setMaxLength(5);

    // Quick connect buttons row (FRANCE / LOCAL)
    float quickBtnY = portsY + LABEL_HEIGHT + INPUT_HEIGHT + 25.0f;
    float quickBtnWidth = (inputWidth - 20.0f) / 2.0f;

    // FRANCE button
    _franceButton = std::make_unique<Button>(
        Vec2f{contentX, quickBtnY},
        Vec2f{quickBtnWidth, 40},
        "FRANCE",
        _fontKey
    );
    _franceButton->setOnClick([this]() {
        applyPreset(true);
    });
    _franceButton->setNormalColor({50, 80, 120, 255});
    _franceButton->setHoveredColor({70, 100, 150, 255});

    // LOCAL button
    _localButton = std::make_unique<Button>(
        Vec2f{contentX + quickBtnWidth + 20.0f, quickBtnY},
        Vec2f{quickBtnWidth, 40},
        "LOCAL",
        _fontKey
    );
    _localButton->setOnClick([this]() {
        applyPreset(false);
    });
    _localButton->setNormalColor({60, 100, 60, 255});
    _localButton->setHoveredColor({80, 130, 80, 255});

    // Bottom buttons row (CONNECTER / ANNULER)
    float buttonsY = quickBtnY + 55.0f;
    float buttonWidth = 150.0f;
    float buttonGap = 20.0f;
    float buttonsStartX = panelX + (PANEL_WIDTH - buttonWidth * 2 - buttonGap) / 2.0f;

    // Connect button
    _connectButton = std::make_unique<Button>(
        Vec2f{buttonsStartX, buttonsY},
        Vec2f{buttonWidth, BUTTON_HEIGHT},
        "CONNECTER",
        _fontKey
    );
    _connectButton->setOnClick([this]() {
        applyConfig();
    });

    // Cancel button
    _cancelButton = std::make_unique<Button>(
        Vec2f{buttonsStartX + buttonWidth + buttonGap, buttonsY},
        Vec2f{buttonWidth, BUTTON_HEIGHT},
        "ANNULER",
        _fontKey
    );
    _cancelButton->setOnClick([this]() {
        if (_onCancel) _onCancel();
    });
    _cancelButton->setNormalColor({80, 60, 60, 255});
    _cancelButton->setHoveredColor({100, 70, 70, 255});

    _initialized = true;
}

void ServerConfigPanel::refreshFromConfig()
{
    auto& serverConfig = config::ServerConfigManager::getInstance();
    if (_hostInput) _hostInput->setText(serverConfig.getHost());
    if (_tcpPortInput) _tcpPortInput->setText(std::to_string(serverConfig.getTcpPort()));
    if (_udpPortInput) _udpPortInput->setText(std::to_string(serverConfig.getUdpPort()));
}

void ServerConfigPanel::applyConfig()
{
    auto& serverConfig = config::ServerConfigManager::getInstance();

    std::string host = _hostInput ? _hostInput->getText() : "127.0.0.1";
    std::string tcpPortStr = _tcpPortInput ? _tcpPortInput->getText() : "4125";
    std::string udpPortStr = _udpPortInput ? _udpPortInput->getText() : "4124";

    // Validate and apply host
    if (!host.empty()) {
        serverConfig.setHost(host);
    }

    // Validate and apply TCP port
    try {
        int tcpPort = std::stoi(tcpPortStr);
        if (tcpPort > 0 && tcpPort <= 65535) {
            serverConfig.setTcpPort(static_cast<uint16_t>(tcpPort));
        }
    } catch (...) {}

    // Validate and apply UDP port
    try {
        int udpPort = std::stoi(udpPortStr);
        if (udpPort > 0 && udpPort <= 65535) {
            serverConfig.setUdpPort(static_cast<uint16_t>(udpPort));
        }
    } catch (...) {}

    // Save config
    serverConfig.save();

    // Trigger callback
    if (_onConnect) _onConnect();
}

void ServerConfigPanel::applyPreset(bool isFrance)
{
    auto& serverConfig = config::ServerConfigManager::getInstance();
    if (isFrance) {
        serverConfig.applyFrancePreset();
    } else {
        serverConfig.applyLocalPreset();
    }
    if (_onConnect) _onConnect();
}

bool ServerConfigPanel::handleEvent(const events::Event& event)
{
    if (_hostInput) _hostInput->handleEvent(event);
    if (_tcpPortInput) _tcpPortInput->handleEvent(event);
    if (_udpPortInput) _udpPortInput->handleEvent(event);
    if (_franceButton) _franceButton->handleEvent(event);
    if (_localButton) _localButton->handleEvent(event);
    if (_connectButton) _connectButton->handleEvent(event);
    if (_cancelButton) _cancelButton->handleEvent(event);

    // Handle clicking outside inputs to unfocus
    if (auto* mousePressed = std::get_if<events::MouseButtonPressed>(&event)) {
        float mx = static_cast<float>(mousePressed->x);
        float my = static_cast<float>(mousePressed->y);

        bool inHost = _hostInput && mx >= _hostInput->getPos().x &&
                     mx <= _hostInput->getPos().x + _hostInput->getSize().x &&
                     my >= _hostInput->getPos().y &&
                     my <= _hostInput->getPos().y + _hostInput->getSize().y;
        bool inTcp = _tcpPortInput && mx >= _tcpPortInput->getPos().x &&
                    mx <= _tcpPortInput->getPos().x + _tcpPortInput->getSize().x &&
                    my >= _tcpPortInput->getPos().y &&
                    my <= _tcpPortInput->getPos().y + _tcpPortInput->getSize().y;
        bool inUdp = _udpPortInput && mx >= _udpPortInput->getPos().x &&
                    mx <= _udpPortInput->getPos().x + _udpPortInput->getSize().x &&
                    my >= _udpPortInput->getPos().y &&
                    my <= _udpPortInput->getPos().y + _udpPortInput->getSize().y;

        if (_hostInput) _hostInput->setFocused(inHost);
        if (_tcpPortInput) _tcpPortInput->setFocused(inTcp);
        if (_udpPortInput) _udpPortInput->setFocused(inUdp);
    }

    // Handle Escape key to cancel
    if (auto* keyPressed = std::get_if<events::KeyPressed>(&event)) {
        if (keyPressed->key == events::Key::Escape) {
            if (_onCancel) _onCancel();
            return true;
        }
    }

    return true;  // Panel consumes all events when visible
}

void ServerConfigPanel::update(float deltaTime)
{
    if (_hostInput) _hostInput->update(deltaTime);
    if (_tcpPortInput) _tcpPortInput->update(deltaTime);
    if (_udpPortInput) _udpPortInput->update(deltaTime);
    if (_franceButton) _franceButton->update(deltaTime);
    if (_localButton) _localButton->update(deltaTime);
    if (_connectButton) _connectButton->update(deltaTime);
    if (_cancelButton) _cancelButton->update(deltaTime);
}

void ServerConfigPanel::render(graphics::IWindow& window)
{
    // Semi-transparent overlay
    window.drawRect(0, 0, _screenWidth, _screenHeight, {0, 0, 0, 180});

    // Panel position (centered)
    float panelX = _screenWidth / 2 - PANEL_WIDTH / 2;
    float panelY = _screenHeight / 2 - 180.0f;

    // Panel background
    window.drawRect(panelX, panelY, PANEL_WIDTH, PANEL_HEIGHT, {25, 25, 45, 250});

    // Panel border
    window.drawRect(panelX, panelY, PANEL_WIDTH, 3, {80, 100, 180, 255});
    window.drawRect(panelX, panelY + PANEL_HEIGHT - 3, PANEL_WIDTH, 3, {80, 100, 180, 255});
    window.drawRect(panelX, panelY, 3, PANEL_HEIGHT, {80, 100, 180, 255});
    window.drawRect(panelX + PANEL_WIDTH - 3, panelY, 3, PANEL_HEIGHT, {80, 100, 180, 255});

    // Title
    window.drawText(_fontKey, "CONFIGURATION SERVEUR",
        _screenWidth / 2 - 140, panelY + 20, 28, {100, 150, 255, 255});

    // Layout variables
    float contentX = panelX + CONTENT_MARGIN;
    float inputWidth = PANEL_WIDTH - CONTENT_MARGIN * 2;
    float inputStartY = panelY + 70.0f;
    float portInputWidth = (inputWidth - 20.0f) / 2.0f;

    // Host label
    window.drawText(_fontKey, "Adresse du serveur:",
        contentX, inputStartY, 18, {180, 180, 200, 255});

    // Render host input
    if (_hostInput) _hostInput->render(window);

    // Ports row
    float portsY = inputStartY + LABEL_HEIGHT + INPUT_HEIGHT + 40.0f;

    // TCP port label
    window.drawText(_fontKey, "Port TCP:",
        contentX, portsY, 18, {180, 180, 200, 255});

    // UDP port label
    window.drawText(_fontKey, "Port UDP:",
        contentX + portInputWidth + 20.0f, portsY, 18, {180, 180, 200, 255});

    // Render port inputs
    if (_tcpPortInput) _tcpPortInput->render(window);
    if (_udpPortInput) _udpPortInput->render(window);

    // Quick connect label
    float quickBtnY = portsY + LABEL_HEIGHT + INPUT_HEIGHT + 25.0f;
    window.drawText(_fontKey, "Connexion rapide:",
        contentX, quickBtnY - 22.0f, 16, {140, 140, 160, 255});

    // Render quick connect buttons
    if (_franceButton) _franceButton->render(window);
    if (_localButton) _localButton->render(window);

    // Render bottom buttons
    if (_connectButton) _connectButton->render(window);
    if (_cancelButton) _cancelButton->render(window);

    // Manual input hint
    window.drawText(_fontKey, "Ou saisir une adresse personnalisee ci-dessus",
        _screenWidth / 2 - 160, panelY + PANEL_HEIGHT - 30, 13, {100, 100, 120, 255});
}

} // namespace ui
