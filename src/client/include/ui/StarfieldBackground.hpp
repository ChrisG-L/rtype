/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** StarfieldBackground - Reusable animated starfield component
*/

#ifndef STARFIELDBACKGROUND_HPP_
#define STARFIELDBACKGROUND_HPP_

#include <vector>
#include <random>
#include "graphics/IWindow.hpp"

namespace ui {

class StarfieldBackground {
public:
    struct Star {
        float x, y;
        float speed;
        float size;
        int brightness;
    };

    StarfieldBackground(float screenWidth, float screenHeight, int starCount = 100);

    void update(float deltaTime);
    void render(graphics::IWindow& window) const;

private:
    void initStars();
    void respawnStar(Star& star);

    float _screenWidth;
    float _screenHeight;
    int _starCount;
    std::vector<Star> _stars;
    std::mt19937 _rng;
    bool _initialized = false;
};

} // namespace ui

#endif /* !STARFIELDBACKGROUND_HPP_ */
