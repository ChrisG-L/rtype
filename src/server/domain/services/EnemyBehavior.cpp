/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** EnemyBehavior - Implementation
*/

#include "domain/services/EnemyBehavior.hpp"
#include "domain/Constants.hpp"
#include <cmath>
#include <algorithm>

namespace domain::services {

    using namespace domain::constants;

    EnemyMovementOutput EnemyBehavior::calculateMovement(const EnemyMovementInput& input) const {
        EnemyMovementOutput output;
        output.newX = input.currentX + getHorizontalMovement(input.enemyType, input.deltaTime);
        output.newBaseY = input.baseY;
        output.zigzag = input.zigzag;

        switch (input.enemyType) {
            case 0:  // Basic
            case 5:  // POWArmor (same pattern as Basic)
            {
                float amplitude = enemy::AMPLITUDE;
                float frequency = enemy::FREQUENCY;
                output.newY = calculateBasicY(input.baseY, amplitude, frequency,
                                              input.aliveTime, input.phaseOffset);
                break;
            }

            case 1:  // Tracker
            {
                float maxMove = enemy::TRACKER_SPEED_Y * input.deltaTime;
                output.newY = calculateTrackerY(input.currentY, input.targetY, maxMove);
                break;
            }

            case 2:  // Zigzag
            {
                auto zigzagResult = calculateZigzagMovement(input);
                output.newY = zigzagResult.newY;
                output.zigzag = zigzagResult.zigzag;
                break;
            }

            case 3:  // Fast
            {
                float amplitude = enemy::AMPLITUDE * 0.6f;
                float frequency = enemy::FREQUENCY * 2.5f;
                output.newY = calculateBasicY(input.baseY, amplitude, frequency,
                                              input.aliveTime, input.phaseOffset);
                break;
            }

            case 4:  // Bomber
            {
                float amplitude = enemy::AMPLITUDE * 0.3f;
                float frequency = enemy::FREQUENCY * 0.5f;
                output.newY = calculateBasicY(input.baseY, amplitude, frequency,
                                              input.aliveTime, input.phaseOffset);
                // Bomber drifts downward slowly
                output.newBaseY = input.baseY + 10.0f * input.deltaTime;
                break;
            }

            default:
                output.newY = input.currentY;
                break;
        }

        // Clamp Y position to screen bounds
        output.newY = std::clamp(output.newY, 0.0f,
                                 world::SCREEN_HEIGHT - enemy::HEIGHT);
        output.newBaseY = std::clamp(output.newBaseY,
                                     wave::SPAWN_Y_MIN, wave::SPAWN_Y_MAX);

        return output;
    }

    float EnemyBehavior::getHorizontalMovement(uint8_t enemyType, float deltaTime) const {
        float speedX = 0.0f;
        switch (enemyType) {
            case 0: speedX = enemy::SPEED_X_BASIC; break;
            case 1: speedX = enemy::SPEED_X_TRACKER; break;
            case 2: speedX = enemy::SPEED_X_ZIGZAG; break;
            case 3: speedX = enemy::SPEED_X_FAST; break;
            case 4: speedX = enemy::SPEED_X_BOMBER; break;
            case 5: speedX = enemy::SPEED_X_POW_ARMOR; break;
            default: speedX = enemy::SPEED_X_BASIC; break;
        }
        return speedX * deltaTime;
    }

    bool EnemyBehavior::canShoot(float shootCooldown) const {
        return shootCooldown <= 0.0f;
    }

    float EnemyBehavior::getMovementAmplitude(uint8_t enemyType) const {
        switch (enemyType) {
            case 0:  // Basic
            case 5:  // POWArmor
                return enemy::AMPLITUDE;
            case 3:  // Fast
                return enemy::AMPLITUDE * 0.6f;
            case 4:  // Bomber
                return enemy::AMPLITUDE * 0.3f;
            default:
                return 0.0f;  // Tracker and Zigzag don't use sinusoidal
        }
    }

    float EnemyBehavior::getMovementFrequency(uint8_t enemyType) const {
        switch (enemyType) {
            case 0:  // Basic
            case 5:  // POWArmor
                return enemy::FREQUENCY;
            case 3:  // Fast
                return enemy::FREQUENCY * 2.5f;
            case 4:  // Bomber
                return enemy::FREQUENCY * 0.5f;
            default:
                return 0.0f;
        }
    }

    bool EnemyBehavior::shouldZigzagFlip(float timer) const {
        return timer >= enemy::ZIGZAG_INTERVAL;
    }

    float EnemyBehavior::getZigzagInterval() const {
        return enemy::ZIGZAG_INTERVAL;
    }

    float EnemyBehavior::getZigzagSpeed() const {
        return enemy::ZIGZAG_SPEED_Y;
    }

    float EnemyBehavior::getTrackerSpeed() const {
        return enemy::TRACKER_SPEED_Y;
    }

    // Private helpers

    float EnemyBehavior::calculateBasicY(float baseY, float amplitude, float frequency,
                                         float aliveTime, float phaseOffset) const {
        return baseY + amplitude * std::sin(frequency * aliveTime + phaseOffset);
    }

    float EnemyBehavior::calculateTrackerY(float currentY, float targetY, float maxMove) const {
        float diff = targetY - currentY;

        if (std::abs(diff) > maxMove) {
            return currentY + (diff > 0 ? maxMove : -maxMove);
        }
        return targetY;
    }

    EnemyMovementOutput EnemyBehavior::calculateZigzagMovement(const EnemyMovementInput& input) const {
        EnemyMovementOutput output;
        output.newX = input.currentX;  // X already handled
        output.newBaseY = input.baseY;
        output.zigzag = input.zigzag;

        // Update zigzag timer
        float newTimer = input.zigzag.timer + input.deltaTime;
        bool goingUp = input.zigzag.goingUp;

        if (newTimer >= enemy::ZIGZAG_INTERVAL) {
            newTimer = 0.0f;
            goingUp = !goingUp;
        }

        output.zigzag.timer = newTimer;
        output.zigzag.goingUp = goingUp;

        // Calculate Y movement
        float direction = goingUp ? -1.0f : 1.0f;
        output.newY = input.currentY + direction * enemy::ZIGZAG_SPEED_Y * input.deltaTime;

        return output;
    }

}  // namespace domain::services
