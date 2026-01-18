/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** EnemyBehavior - Domain service for enemy movement and behavior
*/

#ifndef ENEMYBEHAVIOR_HPP_
#define ENEMYBEHAVIOR_HPP_

#include <cstdint>

namespace domain::services {

    /**
     * @brief Movement delta to apply to an enemy.
     */
    struct MovementDelta {
        float dx;        // Horizontal movement
        float dy;        // Vertical movement
        float newBaseY;  // Updated base Y (for drift patterns like Bomber)
    };

    /**
     * @brief State for zigzag enemies (mutable state managed by caller).
     */
    struct ZigzagState {
        float timer;     // Time since last direction change
        bool goingUp;    // Current direction
    };

    /**
     * @brief Input data for calculating enemy movement.
     */
    struct EnemyMovementInput {
        uint8_t enemyType;
        float currentX;
        float currentY;
        float baseY;
        float aliveTime;
        float phaseOffset;
        float deltaTime;

        // Zigzag-specific state
        ZigzagState zigzag;

        // Tracker-specific: nearest player Y
        float targetY;
    };

    /**
     * @brief Output data from movement calculation.
     */
    struct EnemyMovementOutput {
        float newX;
        float newY;
        float newBaseY;
        ZigzagState zigzag;  // Updated zigzag state
    };

    /**
     * @brief Domain service for enemy movement and behavior calculations.
     *
     * This service calculates enemy movements based on their type:
     * - Basic: Sinusoidal wave pattern
     * - Tracker: Follows nearest player Y position
     * - Zigzag: Alternating up/down movement
     * - Fast: Fast sinusoidal with reduced amplitude
     * - Bomber: Slow sinusoidal with downward drift
     * - POWArmor: Same as Basic
     *
     * All methods are const and stateless - they only depend on their inputs.
     */
    class EnemyBehavior {
    public:
        EnemyBehavior() = default;
        ~EnemyBehavior() = default;

        /**
         * @brief Calculate movement for an enemy.
         * @param input Input data including enemy type, position, and state
         * @return Output data with new position and updated state
         */
        EnemyMovementOutput calculateMovement(const EnemyMovementInput& input) const;

        /**
         * @brief Get horizontal movement delta for an enemy type.
         * @param enemyType 0=Basic, 1=Tracker, etc.
         * @param deltaTime Time elapsed
         * @return Horizontal movement (always negative, moving left)
         */
        float getHorizontalMovement(uint8_t enemyType, float deltaTime) const;

        /**
         * @brief Check if an enemy should shoot.
         * @param shootCooldown Current cooldown value
         * @return true if cooldown <= 0 (can shoot)
         */
        bool canShoot(float shootCooldown) const;

        /**
         * @brief Get movement amplitude for sinusoidal patterns.
         * @param enemyType Enemy type
         * @return Amplitude in pixels
         */
        float getMovementAmplitude(uint8_t enemyType) const;

        /**
         * @brief Get movement frequency for sinusoidal patterns.
         * @param enemyType Enemy type
         * @return Frequency in radians per second
         */
        float getMovementFrequency(uint8_t enemyType) const;

        /**
         * @brief Check if zigzag should flip direction.
         * @param timer Current zigzag timer
         * @return true if should flip direction
         */
        bool shouldZigzagFlip(float timer) const;

        /**
         * @brief Get zigzag flip interval.
         * @return Interval in seconds
         */
        float getZigzagInterval() const;

        /**
         * @brief Get zigzag vertical speed.
         * @return Speed in pixels per second
         */
        float getZigzagSpeed() const;

        /**
         * @brief Get tracker vertical speed.
         * @return Speed in pixels per second
         */
        float getTrackerSpeed() const;

    private:
        float calculateBasicY(float baseY, float amplitude, float frequency,
                              float aliveTime, float phaseOffset) const;

        float calculateTrackerY(float currentY, float targetY,
                                float maxMove) const;

        EnemyMovementOutput calculateZigzagMovement(const EnemyMovementInput& input) const;
    };

}  // namespace domain::services

#endif /* !ENEMYBEHAVIOR_HPP_ */
