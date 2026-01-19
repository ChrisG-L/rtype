/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** BreakoutScene - Mini-game integrated into R-Type client
*/

#ifndef BREAKOUTSCENE_HPP_
#define BREAKOUTSCENE_HPP_

#include "IScene.hpp"
#include "ui/Button.hpp"
#include "ui/StarfieldBackground.hpp"
#include "../utils/Vecs.hpp"
#include "collision/AABB.hpp"
#include <memory>
#include <vector>
#include <unordered_set>
#include <cmath>

class BreakoutScene : public IScene {
public:
    BreakoutScene();
    ~BreakoutScene() override = default;

    void handleEvent(const events::Event& event) override;
    void update(float deltaTime) override;
    void render() override;

private:
    // Initialization
    void loadAssets();
    void initUI();
    void initGame();
    void initBricks();
    void resetBall();

    // Game logic
    void updatePaddle(float deltaTime);
    void updateBall(float deltaTime);
    void checkWallCollisions();
    void checkPaddleCollision();
    void checkBrickCollisions();
    void launchBall();
    void loseLife();
    void nextLevel();
    void restartGame();

    // Rendering
    void renderBackground();
    void renderBricks();
    void renderPaddle();
    void renderBall();
    void renderHUD();
    void renderReady();
    void renderPaused();
    void renderGameOver();

    // Navigation
    void onBackClick();
    void onRestartClick();

    // Utility
    collision::AABB getPaddleAABB() const;
    collision::AABB getBallAABB() const;
    rgba getBrickColor(int row) const;
    int getBrickPoints(int row) const;

    // State flags
    bool _assetsLoaded = false;
    bool _uiInitialized = false;
    bool _gameInitialized = false;

    // Game state
    enum class GameState {
        Ready,      // Ball attached to paddle, waiting for launch
        Playing,    // Game active
        Paused,     // Pause menu shown
        GameOver    // Lost all lives
    };
    GameState _gameState = GameState::Ready;

    // Brick structure
    struct Brick {
        float x;
        float y;
        int row;
        bool alive = true;
    };

    // Paddle
    float _paddleX = 0.0f;

    // Ball
    float _ballX = 0.0f;
    float _ballY = 0.0f;
    float _ballVelX = 0.0f;
    float _ballVelY = 0.0f;
    float _ballSpeed = 0.0f;

    // Bricks
    std::vector<Brick> _bricks;
    int _bricksRemaining = 0;

    // Score and progression
    uint32_t _score = 0;
    uint32_t _highScore = 0;
    int _lives = 3;
    int _level = 1;
    uint32_t _lastExtraLifeScore = 0;

    // Input
    std::unordered_set<events::Key> _keysPressed;
    float _mouseX = 0.0f;
    bool _useMouseControl = false;

    // UI
    std::unique_ptr<ui::Button> _backButton;
    std::unique_ptr<ui::Button> _restartButton;
    std::unique_ptr<ui::Button> _resumeButton;
    std::unique_ptr<ui::StarfieldBackground> _starfield;

    // Constants
    static constexpr float SCREEN_WIDTH = 1920.0f;
    static constexpr float SCREEN_HEIGHT = 1080.0f;
    static constexpr const char* FONT_KEY = "breakout_font";

    // Paddle constants
    static constexpr float PADDLE_WIDTH = 150.0f;
    static constexpr float PADDLE_HEIGHT = 20.0f;
    static constexpr float PADDLE_Y = 950.0f;
    static constexpr float PADDLE_SPEED = 800.0f;

    // Ball constants
    static constexpr float BALL_SIZE = 16.0f;
    static constexpr float BALL_INITIAL_SPEED = 500.0f;
    static constexpr float BALL_MAX_SPEED = 1200.0f;
    static constexpr float BALL_SPEED_INCREMENT = 1.10f;

    // Brick constants
    static constexpr int BRICK_ROWS = 6;
    static constexpr int BRICK_COLS = 12;
    static constexpr float BRICK_WIDTH = 130.0f;
    static constexpr float BRICK_HEIGHT = 35.0f;
    static constexpr float BRICK_SPACING = 5.0f;
    static constexpr float BRICKS_START_Y = 150.0f;

    // Game constants
    static constexpr int INITIAL_LIVES = 3;
    static constexpr int MAX_LIVES = 5;
    static constexpr uint32_t EXTRA_LIFE_SCORE = 10000;
    static constexpr int LEVEL_BONUS = 500;
    static constexpr float MAX_BOUNCE_ANGLE = 60.0f * 3.14159f / 180.0f;
};

#endif /* !BREAKOUTSCENE_HPP_ */
