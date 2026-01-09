/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** StarfieldBackground
*/

#include "ui/StarfieldBackground.hpp"

namespace ui {

StarfieldBackground::StarfieldBackground(float screenWidth, float screenHeight, int starCount)
    : _screenWidth(screenWidth), _screenHeight(screenHeight), _starCount(starCount)
{
    std::random_device rd;
    _rng.seed(rd());
    initStars();
}

void StarfieldBackground::initStars()
{
    if (_initialized) return;

    std::uniform_real_distribution<float> xDist(0, _screenWidth);
    std::uniform_real_distribution<float> yDist(0, _screenHeight);
    std::uniform_real_distribution<float> speedDist(15.0f, 100.0f);
    std::uniform_real_distribution<float> sizeDist(1.0f, 3.0f);
    std::uniform_int_distribution<int> brightnessDist(80, 255);

    _stars.reserve(_starCount);
    for (int i = 0; i < _starCount; ++i) {
        _stars.push_back({
            xDist(_rng),
            yDist(_rng),
            speedDist(_rng),
            sizeDist(_rng),
            brightnessDist(_rng)
        });
    }
    _initialized = true;
}

void StarfieldBackground::respawnStar(Star& star)
{
    std::uniform_real_distribution<float> yDist(0, _screenHeight);
    std::uniform_real_distribution<float> speedDist(15.0f, 100.0f);
    std::uniform_real_distribution<float> sizeDist(1.0f, 3.0f);
    std::uniform_int_distribution<int> brightnessDist(80, 255);

    star.x = _screenWidth;
    star.y = yDist(_rng);
    star.speed = speedDist(_rng);
    star.size = sizeDist(_rng);
    star.brightness = brightnessDist(_rng);
}

void StarfieldBackground::update(float deltaTime)
{
    for (auto& star : _stars) {
        star.x -= star.speed * deltaTime;
        if (star.x < 0) {
            respawnStar(star);
        }
    }
}

void StarfieldBackground::render(graphics::IWindow& window) const
{
    for (const auto& star : _stars) {
        window.drawRect(star.x, star.y, star.size, star.size,
            {static_cast<uint8_t>(star.brightness),
             static_cast<uint8_t>(star.brightness),
             static_cast<uint8_t>(star.brightness), 255});
    }
}

} // namespace ui
