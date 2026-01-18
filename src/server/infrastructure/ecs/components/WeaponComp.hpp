/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** WeaponComp - ECS Component for weapon state
*/

#ifndef WEAPON_COMP_HPP_
#define WEAPON_COMP_HPP_

#include <cstdint>
#include <array>

namespace infrastructure::ecs::components {

    /**
     * @brief Component storing weapon state for players.
     *
     * currentType: Currently selected weapon (0-3: Standard, Spread, Laser, Missile)
     * shootCooldown: Time until next shot allowed (seconds)
     * isCharging: Currently charging Wave Cannon
     * chargeTime: Time spent charging (seconds)
     * weaponLevels: Independent upgrade levels per weapon (0-3 each)
     */
    struct WeaponComp {
        uint8_t currentType = 0;                       // WeaponType enum (0-3)
        float shootCooldown = 0.0f;                    // Cooldown remaining (seconds)
        bool isCharging = false;                       // Wave Cannon charging
        float chargeTime = 0.0f;                       // Charge accumulator (seconds)
        std::array<uint8_t, 4> weaponLevels = {0, 0, 0, 0};  // Per-weapon levels

        /**
         * @brief Get current weapon's level.
         */
        uint8_t getCurrentLevel() const {
            if (currentType < 4) {
                return weaponLevels[currentType];
            }
            return 0;
        }

        /**
         * @brief Upgrade current weapon's level.
         * @return true if upgraded, false if already max (3)
         */
        bool upgradeCurrentWeapon() {
            if (currentType < 4 && weaponLevels[currentType] < 3) {
                weaponLevels[currentType]++;
                return true;
            }
            return false;
        }
    };

}  // namespace infrastructure::ecs::components

#endif /* !WEAPON_COMP_HPP_ */
