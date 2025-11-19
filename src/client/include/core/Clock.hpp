/*
** EPITECH PROJECT, 2025
** R-Type
** File description:
** Clock - Horloge de haute précision
*/

#pragma once

#include <chrono>

namespace rtype::core {

/**
 * @brief Horloge de haute précision pour le game loop
 */
class Clock {
public:
    using TimePoint = std::chrono::high_resolution_clock::time_point;
    using Duration = std::chrono::duration<float>;

    Clock() : m_start(std::chrono::high_resolution_clock::now()),
              m_lastTick(m_start) {}

    /**
     * @brief Redémarre l'horloge et retourne le temps écoulé
     */
    float restart() {
        auto now = std::chrono::high_resolution_clock::now();
        Duration elapsed = now - m_lastTick;
        m_lastTick = now;
        return elapsed.count();
    }

    /**
     * @brief Retourne le temps écoulé depuis le dernier restart
     */
    [[nodiscard]] float getElapsed() const {
        auto now = std::chrono::high_resolution_clock::now();
        Duration elapsed = now - m_lastTick;
        return elapsed.count();
    }

    /**
     * @brief Retourne le temps total depuis la création
     */
    [[nodiscard]] float getTotalTime() const {
        auto now = std::chrono::high_resolution_clock::now();
        Duration elapsed = now - m_start;
        return elapsed.count();
    }

private:
    TimePoint m_start;
    TimePoint m_lastTick;
};

} // namespace rtype::core
