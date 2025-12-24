/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** MainMenuScene
*/

#ifndef MAINMENUSCENE_HPP_
#define MAINMENUSCENE_HPP_

#include "IScene.hpp"
#include "ui/Button.hpp"
#include "ui/StarfieldBackground.hpp"
#include "../utils/Vecs.hpp"
#include <memory>

class MainMenuScene : public IScene {
public:
    MainMenuScene();
    ~MainMenuScene() override = default;

    void handleEvent(const events::Event& event) override;
    void update(float deltaTime) override;
    void render() override;

private:
    void initUI();
    void loadAssets();
    void onPlayClick();
    void onSettingsClick();
    void onQuitClick();

    bool _assetsLoaded = false;
    bool _uiInitialized = false;

    std::unique_ptr<ui::Button> _playButton;
    std::unique_ptr<ui::Button> _settingsButton;
    std::unique_ptr<ui::Button> _quitButton;

    // Animated starfield background
    std::unique_ptr<ui::StarfieldBackground> _starfield;

    static constexpr float SCREEN_WIDTH = 1920.0f;
    static constexpr float SCREEN_HEIGHT = 1080.0f;
    static constexpr const char* FONT_KEY = "menu_font";
    static constexpr int STAR_COUNT = 150;
};

#endif /* !MAINMENUSCENE_HPP_ */
