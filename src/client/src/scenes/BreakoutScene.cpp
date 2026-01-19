/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** BreakoutScene - Mini-game implementation
*/

#include "scenes/BreakoutScene.hpp"
#include "scenes/SceneManager.hpp"
#include "scenes/MainMenuScene.hpp"
#include "audio/AudioManager.hpp"
#include <algorithm>
#include <cmath>

BreakoutScene::BreakoutScene()
{
    _paddleX = (SCREEN_WIDTH - PADDLE_WIDTH) / 2.0f;
    _ballSpeed = BALL_INITIAL_SPEED;
}

void BreakoutScene::loadAssets()
{
    if (_assetsLoaded || !_context.window) return;

    _context.window->loadFont(FONT_KEY, "assets/fonts/ARIA.TTF");
    _assetsLoaded = true;
}

void BreakoutScene::initUI()
{
    if (_uiInitialized || !_context.window) return;

    // Starfield background
    _starfield = std::make_unique<ui::StarfieldBackground>(
        static_cast<int>(SCREEN_WIDTH),
        static_cast<int>(SCREEN_HEIGHT),
        100
    );

    // Back button (top-left)
    _backButton = std::make_unique<ui::Button>(
        Vec2f{20.0f, 20.0f},
        Vec2f{100.0f, 40.0f},
        "BACK",
        FONT_KEY
    );
    _backButton->setOnClick([this]() { onBackClick(); });
    _backButton->setNormalColor({80, 80, 100, 255});
    _backButton->setHoveredColor({100, 100, 130, 255});

    // Restart button (shown in GameOver/Paused)
    _restartButton = std::make_unique<ui::Button>(
        Vec2f{SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 + 50},
        Vec2f{200.0f, 50.0f},
        "RESTART",
        FONT_KEY
    );
    _restartButton->setOnClick([this]() { onRestartClick(); });
    _restartButton->setNormalColor({50, 150, 50, 255});
    _restartButton->setHoveredColor({70, 180, 70, 255});

    // Resume button (shown in Paused)
    _resumeButton = std::make_unique<ui::Button>(
        Vec2f{SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 20},
        Vec2f{200.0f, 50.0f},
        "RESUME",
        FONT_KEY
    );
    _resumeButton->setOnClick([this]() { _gameState = GameState::Playing; });
    _resumeButton->setNormalColor({50, 100, 150, 255});
    _resumeButton->setHoveredColor({70, 130, 180, 255});

    _uiInitialized = true;
}

void BreakoutScene::initGame()
{
    if (_gameInitialized) return;

    _score = 0;
    _lives = INITIAL_LIVES;
    _level = 1;
    _lastExtraLifeScore = 0;
    _ballSpeed = BALL_INITIAL_SPEED;
    _gameState = GameState::Ready;

    initBricks();
    resetBall();

    _gameInitialized = true;
}

void BreakoutScene::initBricks()
{
    _bricks.clear();

    float totalWidth = BRICK_COLS * (BRICK_WIDTH + BRICK_SPACING) - BRICK_SPACING;
    float startX = (SCREEN_WIDTH - totalWidth) / 2.0f;

    for (int row = 0; row < BRICK_ROWS; ++row) {
        for (int col = 0; col < BRICK_COLS; ++col) {
            Brick brick;
            brick.x = startX + col * (BRICK_WIDTH + BRICK_SPACING);
            brick.y = BRICKS_START_Y + row * (BRICK_HEIGHT + BRICK_SPACING);
            brick.row = row;
            brick.alive = true;
            _bricks.push_back(brick);
        }
    }

    _bricksRemaining = BRICK_ROWS * BRICK_COLS;
}

void BreakoutScene::resetBall()
{
    _ballX = _paddleX + PADDLE_WIDTH / 2.0f - BALL_SIZE / 2.0f;
    _ballY = PADDLE_Y - BALL_SIZE - 2.0f;
    _ballVelX = 0.0f;
    _ballVelY = 0.0f;
}

void BreakoutScene::handleEvent(const events::Event& event)
{
    // Key press events
    if (auto* keyPressed = std::get_if<events::KeyPressed>(&event)) {
        _keysPressed.insert(keyPressed->key);
        _useMouseControl = false;

        if (keyPressed->key == events::Key::Space) {
            if (_gameState == GameState::Ready) {
                launchBall();
            }
        }

        if (keyPressed->key == events::Key::Escape) {
            if (_gameState == GameState::Playing) {
                _gameState = GameState::Paused;
            } else if (_gameState == GameState::Paused) {
                _gameState = GameState::Playing;
            } else if (_gameState == GameState::Ready || _gameState == GameState::GameOver) {
                onBackClick();
            }
        }

        if (keyPressed->key == events::Key::P && _gameState == GameState::Playing) {
            _gameState = GameState::Paused;
        }
    }

    // Key release events
    if (auto* keyReleased = std::get_if<events::KeyReleased>(&event)) {
        _keysPressed.erase(keyReleased->key);
    }

    // Mouse movement
    if (auto* mouseMoved = std::get_if<events::MouseMoved>(&event)) {
        _mouseX = static_cast<float>(mouseMoved->x);
        _useMouseControl = true;
    }

    // UI button events
    _backButton->handleEvent(event);

    if (_gameState == GameState::GameOver) {
        _restartButton->handleEvent(event);
    }

    if (_gameState == GameState::Paused) {
        _resumeButton->handleEvent(event);
        _restartButton->handleEvent(event);
    }
}

void BreakoutScene::update(float deltaTime)
{
    if (!_assetsLoaded) loadAssets();
    if (!_uiInitialized) initUI();
    if (!_gameInitialized) initGame();

    // Update starfield
    if (_starfield) {
        _starfield->update(deltaTime);
    }

    // Update UI buttons
    _backButton->update(deltaTime);

    if (_gameState == GameState::GameOver) {
        _restartButton->update(deltaTime);
    }

    if (_gameState == GameState::Paused) {
        _resumeButton->update(deltaTime);
        _restartButton->update(deltaTime);
    }

    // Game logic
    if (_gameState == GameState::Playing) {
        updatePaddle(deltaTime);
        updateBall(deltaTime);

        // Check for extra life
        if (_score >= _lastExtraLifeScore + EXTRA_LIFE_SCORE && _lives < MAX_LIVES) {
            _lives++;
            _lastExtraLifeScore = (_score / EXTRA_LIFE_SCORE) * EXTRA_LIFE_SCORE;
        }
    } else if (_gameState == GameState::Ready) {
        updatePaddle(deltaTime);
        // Ball follows paddle
        _ballX = _paddleX + PADDLE_WIDTH / 2.0f - BALL_SIZE / 2.0f;
        _ballY = PADDLE_Y - BALL_SIZE - 2.0f;
    }
}

void BreakoutScene::updatePaddle(float deltaTime)
{
    if (_useMouseControl) {
        _paddleX = _mouseX - PADDLE_WIDTH / 2.0f;
    } else {
        for (const auto& key : _keysPressed) {
            if (key == events::Key::Left || key == events::Key::A) {
                _paddleX -= PADDLE_SPEED * deltaTime;
            }
            if (key == events::Key::Right || key == events::Key::D) {
                _paddleX += PADDLE_SPEED * deltaTime;
            }
        }
    }

    // Clamp paddle to screen bounds
    _paddleX = std::clamp(_paddleX, 0.0f, SCREEN_WIDTH - PADDLE_WIDTH);
}

void BreakoutScene::updateBall(float deltaTime)
{
    _ballX += _ballVelX * deltaTime;
    _ballY += _ballVelY * deltaTime;

    checkWallCollisions();
    checkPaddleCollision();
    checkBrickCollisions();

    // Ball out of bounds (bottom)
    if (_ballY > SCREEN_HEIGHT) {
        loseLife();
    }
}

void BreakoutScene::checkWallCollisions()
{
    // Left wall
    if (_ballX < 0) {
        _ballX = 0;
        _ballVelX = -_ballVelX;
    }

    // Right wall
    if (_ballX + BALL_SIZE > SCREEN_WIDTH) {
        _ballX = SCREEN_WIDTH - BALL_SIZE;
        _ballVelX = -_ballVelX;
    }

    // Top wall
    if (_ballY < 0) {
        _ballY = 0;
        _ballVelY = -_ballVelY;
    }
}

void BreakoutScene::checkPaddleCollision()
{
    collision::AABB ballBox = getBallAABB();
    collision::AABB paddleBox = getPaddleAABB();

    if (ballBox.intersects(paddleBox) && _ballVelY > 0) {
        // Calculate hit position (-1 to +1)
        float ballCenterX = _ballX + BALL_SIZE / 2.0f;
        float paddleCenterX = _paddleX + PADDLE_WIDTH / 2.0f;
        float hitPosition = (ballCenterX - paddleCenterX) / (PADDLE_WIDTH / 2.0f);
        hitPosition = std::clamp(hitPosition, -1.0f, 1.0f);

        // Calculate bounce angle
        float bounceAngle = hitPosition * MAX_BOUNCE_ANGLE;

        _ballVelX = _ballSpeed * std::sin(bounceAngle);
        _ballVelY = -std::abs(_ballSpeed * std::cos(bounceAngle));

        // Ensure ball is above paddle
        _ballY = PADDLE_Y - BALL_SIZE - 1.0f;
    }
}

void BreakoutScene::checkBrickCollisions()
{
    collision::AABB ballBox = getBallAABB();

    for (auto& brick : _bricks) {
        if (!brick.alive) continue;

        collision::AABB brickBox(brick.x, brick.y, BRICK_WIDTH, BRICK_HEIGHT);

        if (ballBox.intersects(brickBox)) {
            brick.alive = false;
            _bricksRemaining--;
            _score += getBrickPoints(brick.row);

            // Update high score
            if (_score > _highScore) {
                _highScore = _score;
            }

            // Determine collision side for bounce
            float overlapLeft = (_ballX + BALL_SIZE) - brick.x;
            float overlapRight = (brick.x + BRICK_WIDTH) - _ballX;
            float overlapTop = (_ballY + BALL_SIZE) - brick.y;
            float overlapBottom = (brick.y + BRICK_HEIGHT) - _ballY;

            float minOverlapX = std::min(overlapLeft, overlapRight);
            float minOverlapY = std::min(overlapTop, overlapBottom);

            if (minOverlapX < minOverlapY) {
                _ballVelX = -_ballVelX;
            } else {
                _ballVelY = -_ballVelY;
            }

            // Only break one brick per frame
            break;
        }
    }

    // Check for level complete
    if (_bricksRemaining <= 0) {
        nextLevel();
    }
}

void BreakoutScene::launchBall()
{
    _gameState = GameState::Playing;

    // Launch at a slight angle
    float angle = (static_cast<float>(rand() % 60) - 30.0f) * 3.14159f / 180.0f;
    _ballVelX = _ballSpeed * std::sin(angle);
    _ballVelY = -_ballSpeed * std::cos(angle);
}

void BreakoutScene::loseLife()
{
    _lives--;

    if (_lives <= 0) {
        _gameState = GameState::GameOver;
    } else {
        _gameState = GameState::Ready;
        resetBall();
    }
}

void BreakoutScene::nextLevel()
{
    _level++;
    _score += LEVEL_BONUS * _level;

    // Increase ball speed (capped)
    _ballSpeed = std::min(_ballSpeed * BALL_SPEED_INCREMENT, BALL_MAX_SPEED);

    initBricks();
    _gameState = GameState::Ready;
    resetBall();
}

void BreakoutScene::restartGame()
{
    _gameInitialized = false;
    initGame();
}

void BreakoutScene::onBackClick()
{
    if (_sceneManager) {
        _sceneManager->changeScene(std::make_unique<MainMenuScene>());
    }
}

void BreakoutScene::onRestartClick()
{
    restartGame();
}

collision::AABB BreakoutScene::getPaddleAABB() const
{
    return collision::AABB(_paddleX, PADDLE_Y, PADDLE_WIDTH, PADDLE_HEIGHT);
}

collision::AABB BreakoutScene::getBallAABB() const
{
    return collision::AABB(_ballX, _ballY, BALL_SIZE, BALL_SIZE);
}

rgba BreakoutScene::getBrickColor(int row) const
{
    static const rgba colors[] = {
        {220, 50, 50, 255},    // Red
        {255, 140, 0, 255},    // Orange
        {255, 220, 0, 255},    // Yellow
        {50, 200, 50, 255},    // Green
        {50, 200, 220, 255},   // Cyan
        {50, 100, 220, 255}    // Blue
    };
    return colors[row % 6];
}

int BreakoutScene::getBrickPoints(int row) const
{
    static const int points[] = {70, 60, 50, 40, 30, 20};
    return points[row % 6];
}

void BreakoutScene::render()
{
    if (!_context.window) return;

    renderBackground();
    renderBricks();
    renderPaddle();
    renderBall();
    renderHUD();

    // State overlays
    if (_gameState == GameState::Ready) {
        renderReady();
    } else if (_gameState == GameState::Paused) {
        renderPaused();
    } else if (_gameState == GameState::GameOver) {
        renderGameOver();
    }

    // Back button always visible
    _backButton->render(*_context.window);
}

void BreakoutScene::renderBackground()
{
    // Dark blue background
    _context.window->drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, {5, 5, 20, 255});

    // Starfield
    if (_starfield) {
        _starfield->render(*_context.window);
    }
}

void BreakoutScene::renderBricks()
{
    for (const auto& brick : _bricks) {
        if (!brick.alive) continue;

        rgba color = getBrickColor(brick.row);
        _context.window->drawRect(brick.x, brick.y, BRICK_WIDTH, BRICK_HEIGHT, color);

        // Brick border (darker shade for depth)
        rgba borderColor{
            static_cast<int>(color.r * 0.6f),
            static_cast<int>(color.g * 0.6f),
            static_cast<int>(color.b * 0.6f),
            255
        };
        _context.window->drawRect(brick.x, brick.y + BRICK_HEIGHT - 4, BRICK_WIDTH, 4, borderColor);
    }
}

void BreakoutScene::renderPaddle()
{
    // Paddle glow effect
    _context.window->drawRect(
        _paddleX - 2, PADDLE_Y - 2,
        PADDLE_WIDTH + 4, PADDLE_HEIGHT + 4,
        {100, 200, 255, 100}
    );

    // Main paddle
    _context.window->drawRect(_paddleX, PADDLE_Y, PADDLE_WIDTH, PADDLE_HEIGHT, {100, 200, 255, 255});

    // Paddle highlight
    _context.window->drawRect(_paddleX, PADDLE_Y, PADDLE_WIDTH, 4, {150, 230, 255, 255});
}

void BreakoutScene::renderBall()
{
    // Ball glow
    _context.window->drawRect(
        _ballX - 3, _ballY - 3,
        BALL_SIZE + 6, BALL_SIZE + 6,
        {255, 255, 200, 80}
    );

    // Main ball
    _context.window->drawRect(_ballX, _ballY, BALL_SIZE, BALL_SIZE, {255, 255, 255, 255});
}

void BreakoutScene::renderHUD()
{
    // Score (top-left, after back button)
    std::string scoreStr = "SCORE: " + std::to_string(_score);
    _context.window->drawText(FONT_KEY, scoreStr, 140, 25, 24, {255, 255, 255, 255});

    // High score (top-center)
    std::string highStr = "HIGH: " + std::to_string(_highScore);
    float highX = SCREEN_WIDTH / 2 - 80;
    _context.window->drawText(FONT_KEY, highStr, highX, 25, 24, {255, 215, 0, 255});

    // Level (top-right)
    std::string levelStr = "LEVEL: " + std::to_string(_level);
    _context.window->drawText(FONT_KEY, levelStr, SCREEN_WIDTH - 180, 25, 24, {255, 255, 255, 255});

    // Lives (bottom-left as paddle icons)
    _context.window->drawText(FONT_KEY, "LIVES:", 20, SCREEN_HEIGHT - 50, 18, {200, 200, 200, 255});
    for (int i = 0; i < _lives; ++i) {
        float lifeX = 100 + i * 45;
        _context.window->drawRect(lifeX, SCREEN_HEIGHT - 50, 35, 12, {100, 200, 255, 255});
    }
}

void BreakoutScene::renderReady()
{
    // Semi-transparent overlay
    _context.window->drawRect(0, SCREEN_HEIGHT / 2 - 60, SCREEN_WIDTH, 120, {0, 0, 0, 150});

    // Ready message
    _context.window->drawText(
        FONT_KEY,
        "PRESS SPACE TO LAUNCH",
        SCREEN_WIDTH / 2 - 180,
        SCREEN_HEIGHT / 2 - 20,
        32,
        {255, 255, 255, 255}
    );

    // Controls hint
    _context.window->drawText(
        FONT_KEY,
        "Use LEFT/RIGHT arrows or MOUSE to move paddle",
        SCREEN_WIDTH / 2 - 280,
        SCREEN_HEIGHT / 2 + 25,
        20,
        {180, 180, 180, 255}
    );
}

void BreakoutScene::renderPaused()
{
    // Darken background
    _context.window->drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, {0, 0, 0, 180});

    // Pause title
    _context.window->drawText(
        FONT_KEY,
        "PAUSED",
        SCREEN_WIDTH / 2 - 80,
        SCREEN_HEIGHT / 2 - 100,
        48,
        {255, 255, 255, 255}
    );

    // Buttons
    _resumeButton->render(*_context.window);
    _restartButton->render(*_context.window);
}

void BreakoutScene::renderGameOver()
{
    // Darken background
    _context.window->drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, {0, 0, 0, 200});

    // Game Over title
    _context.window->drawText(
        FONT_KEY,
        "GAME OVER",
        SCREEN_WIDTH / 2 - 120,
        SCREEN_HEIGHT / 2 - 120,
        48,
        {255, 50, 50, 255}
    );

    // Final score
    std::string finalScore = "FINAL SCORE: " + std::to_string(_score);
    _context.window->drawText(
        FONT_KEY,
        finalScore,
        SCREEN_WIDTH / 2 - 140,
        SCREEN_HEIGHT / 2 - 50,
        28,
        {255, 255, 255, 255}
    );

    // Level reached
    std::string levelReached = "LEVEL REACHED: " + std::to_string(_level);
    _context.window->drawText(
        FONT_KEY,
        levelReached,
        SCREEN_WIDTH / 2 - 130,
        SCREEN_HEIGHT / 2 - 10,
        24,
        {200, 200, 200, 255}
    );

    // Restart button
    _restartButton->render(*_context.window);
}
