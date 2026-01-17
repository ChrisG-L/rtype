/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** WeaponLevelsTest - Tests pour les niveaux d'armes indépendants
*/

#include <gtest/gtest.h>
#include <array>
#include <cstdint>

// Tests pour la logique des niveaux d'armes indépendants
// Chaque arme (Standard, Spread, Laser, Missile) a son propre niveau 0-3

// ═══════════════════════════════════════════════════════════════════
// WeaponType enum (copié de Protocol.hpp pour les tests)
// ═══════════════════════════════════════════════════════════════════

enum class WeaponType : uint8_t {
    Standard = 0,
    Spread = 1,
    Laser = 2,
    Missile = 3,
    COUNT = 4
};

// ═══════════════════════════════════════════════════════════════════
// Structure simplifiée pour tester la logique des niveaux d'armes
// ═══════════════════════════════════════════════════════════════════

struct PlayerWeaponState {
    WeaponType currentWeapon = WeaponType::Standard;
    std::array<uint8_t, 4> weaponLevels = {0, 0, 0, 0};

    uint8_t getCurrentWeaponLevel() const {
        return weaponLevels[static_cast<size_t>(currentWeapon)];
    }

    bool upgradeCurrentWeapon() {
        size_t idx = static_cast<size_t>(currentWeapon);
        if (idx < weaponLevels.size() && weaponLevels[idx] < 3) {
            weaponLevels[idx]++;
            return true;
        }
        return false;
    }

    void switchWeapon(WeaponType newWeapon) {
        currentWeapon = newWeapon;
    }
};

// ═══════════════════════════════════════════════════════════════════
// Tests d'initialisation
// ═══════════════════════════════════════════════════════════════════

class WeaponLevelsTest : public ::testing::Test {
protected:
    PlayerWeaponState player;
};

TEST_F(WeaponLevelsTest, DefaultInit_AllLevelsAtZero) {
    EXPECT_EQ(player.weaponLevels[0], 0);
    EXPECT_EQ(player.weaponLevels[1], 0);
    EXPECT_EQ(player.weaponLevels[2], 0);
    EXPECT_EQ(player.weaponLevels[3], 0);
}

TEST_F(WeaponLevelsTest, DefaultInit_StartsWithStandardWeapon) {
    EXPECT_EQ(player.currentWeapon, WeaponType::Standard);
}

TEST_F(WeaponLevelsTest, GetCurrentLevel_ReturnsCorrectLevel) {
    player.weaponLevels[0] = 2;  // Standard
    player.weaponLevels[1] = 1;  // Spread
    player.weaponLevels[2] = 3;  // Laser
    player.weaponLevels[3] = 0;  // Missile

    player.currentWeapon = WeaponType::Standard;
    EXPECT_EQ(player.getCurrentWeaponLevel(), 2);

    player.currentWeapon = WeaponType::Spread;
    EXPECT_EQ(player.getCurrentWeaponLevel(), 1);

    player.currentWeapon = WeaponType::Laser;
    EXPECT_EQ(player.getCurrentWeaponLevel(), 3);

    player.currentWeapon = WeaponType::Missile;
    EXPECT_EQ(player.getCurrentWeaponLevel(), 0);
}

// ═══════════════════════════════════════════════════════════════════
// Tests d'upgrade indépendants
// ═══════════════════════════════════════════════════════════════════

TEST_F(WeaponLevelsTest, Upgrade_OnlyAffectsCurrentWeapon) {
    player.currentWeapon = WeaponType::Spread;
    player.upgradeCurrentWeapon();

    EXPECT_EQ(player.weaponLevels[0], 0);  // Standard unchanged
    EXPECT_EQ(player.weaponLevels[1], 1);  // Spread upgraded
    EXPECT_EQ(player.weaponLevels[2], 0);  // Laser unchanged
    EXPECT_EQ(player.weaponLevels[3], 0);  // Missile unchanged
}

TEST_F(WeaponLevelsTest, Upgrade_MultipleWeaponsIndependent) {
    // Upgrade Standard twice
    player.currentWeapon = WeaponType::Standard;
    player.upgradeCurrentWeapon();
    player.upgradeCurrentWeapon();

    // Upgrade Laser once
    player.currentWeapon = WeaponType::Laser;
    player.upgradeCurrentWeapon();

    // Upgrade Missile three times
    player.currentWeapon = WeaponType::Missile;
    player.upgradeCurrentWeapon();
    player.upgradeCurrentWeapon();
    player.upgradeCurrentWeapon();

    EXPECT_EQ(player.weaponLevels[0], 2);  // Standard: 2
    EXPECT_EQ(player.weaponLevels[1], 0);  // Spread: 0 (never upgraded)
    EXPECT_EQ(player.weaponLevels[2], 1);  // Laser: 1
    EXPECT_EQ(player.weaponLevels[3], 3);  // Missile: 3 (max)
}

TEST_F(WeaponLevelsTest, Upgrade_MaxLevelIs3) {
    player.currentWeapon = WeaponType::Standard;

    EXPECT_TRUE(player.upgradeCurrentWeapon());   // 0 -> 1
    EXPECT_TRUE(player.upgradeCurrentWeapon());   // 1 -> 2
    EXPECT_TRUE(player.upgradeCurrentWeapon());   // 2 -> 3
    EXPECT_FALSE(player.upgradeCurrentWeapon());  // 3 -> blocked

    EXPECT_EQ(player.weaponLevels[0], 3);  // Stays at max
}

TEST_F(WeaponLevelsTest, Upgrade_EachWeaponHasOwnMax) {
    // Max out all weapons
    for (int w = 0; w < 4; w++) {
        player.currentWeapon = static_cast<WeaponType>(w);
        for (int i = 0; i < 5; i++) {  // Try 5 upgrades
            player.upgradeCurrentWeapon();
        }
    }

    EXPECT_EQ(player.weaponLevels[0], 3);
    EXPECT_EQ(player.weaponLevels[1], 3);
    EXPECT_EQ(player.weaponLevels[2], 3);
    EXPECT_EQ(player.weaponLevels[3], 3);
}

// ═══════════════════════════════════════════════════════════════════
// Tests de changement d'arme
// ═══════════════════════════════════════════════════════════════════

TEST_F(WeaponLevelsTest, SwitchWeapon_PreservesLevels) {
    player.currentWeapon = WeaponType::Standard;
    player.upgradeCurrentWeapon();  // Standard = 1
    player.upgradeCurrentWeapon();  // Standard = 2

    player.switchWeapon(WeaponType::Laser);
    player.upgradeCurrentWeapon();  // Laser = 1

    // Switch back to Standard
    player.switchWeapon(WeaponType::Standard);
    EXPECT_EQ(player.getCurrentWeaponLevel(), 2);  // Still level 2

    // Switch to Laser
    player.switchWeapon(WeaponType::Laser);
    EXPECT_EQ(player.getCurrentWeaponLevel(), 1);  // Still level 1
}

TEST_F(WeaponLevelsTest, SwitchWeapon_FrequentSwitching) {
    // Setup different levels
    player.weaponLevels = {1, 2, 3, 0};

    // Rapid switching should always show correct level
    for (int i = 0; i < 10; i++) {
        player.switchWeapon(WeaponType::Standard);
        EXPECT_EQ(player.getCurrentWeaponLevel(), 1);

        player.switchWeapon(WeaponType::Spread);
        EXPECT_EQ(player.getCurrentWeaponLevel(), 2);

        player.switchWeapon(WeaponType::Laser);
        EXPECT_EQ(player.getCurrentWeaponLevel(), 3);

        player.switchWeapon(WeaponType::Missile);
        EXPECT_EQ(player.getCurrentWeaponLevel(), 0);
    }
}

// ═══════════════════════════════════════════════════════════════════
// Tests du bug original (régression)
// ═══════════════════════════════════════════════════════════════════

TEST_F(WeaponLevelsTest, Regression_UpgradeStandardDoesNotAffectSpread) {
    // Bug original: upgrader Standard upgradeait toutes les armes
    player.currentWeapon = WeaponType::Standard;
    player.upgradeCurrentWeapon();
    player.upgradeCurrentWeapon();
    player.upgradeCurrentWeapon();  // Standard = 3

    player.switchWeapon(WeaponType::Spread);
    // BUG: Spread devait être 0, pas 3
    EXPECT_EQ(player.getCurrentWeaponLevel(), 0);
}

TEST_F(WeaponLevelsTest, Regression_EachWeaponStartsAtZero) {
    // Après avoir upgradé une arme, les autres restent à 0
    player.currentWeapon = WeaponType::Missile;
    player.upgradeCurrentWeapon();
    player.upgradeCurrentWeapon();

    // Toutes les autres armes doivent être à 0
    player.switchWeapon(WeaponType::Standard);
    EXPECT_EQ(player.getCurrentWeaponLevel(), 0);

    player.switchWeapon(WeaponType::Spread);
    EXPECT_EQ(player.getCurrentWeaponLevel(), 0);

    player.switchWeapon(WeaponType::Laser);
    EXPECT_EQ(player.getCurrentWeaponLevel(), 0);
}

// ═══════════════════════════════════════════════════════════════════
// Tests de snapshot (simulation du protocole réseau)
// ═══════════════════════════════════════════════════════════════════

TEST_F(WeaponLevelsTest, Snapshot_ReturnsCurrentWeaponLevel) {
    player.weaponLevels = {0, 2, 1, 3};

    // Le snapshot envoie uniquement le niveau de l'arme actuelle
    player.currentWeapon = WeaponType::Standard;
    uint8_t snapshotLevel = player.getCurrentWeaponLevel();
    EXPECT_EQ(snapshotLevel, 0);

    player.currentWeapon = WeaponType::Spread;
    snapshotLevel = player.getCurrentWeaponLevel();
    EXPECT_EQ(snapshotLevel, 2);

    player.currentWeapon = WeaponType::Laser;
    snapshotLevel = player.getCurrentWeaponLevel();
    EXPECT_EQ(snapshotLevel, 1);

    player.currentWeapon = WeaponType::Missile;
    snapshotLevel = player.getCurrentWeaponLevel();
    EXPECT_EQ(snapshotLevel, 3);
}

// ═══════════════════════════════════════════════════════════════════
// Tests de cas limites
// ═══════════════════════════════════════════════════════════════════

TEST_F(WeaponLevelsTest, EdgeCase_ArrayBounds) {
    // Vérifier que les indices sont valides
    for (int w = 0; w < static_cast<int>(WeaponType::COUNT); w++) {
        player.currentWeapon = static_cast<WeaponType>(w);
        EXPECT_NO_THROW(player.getCurrentWeaponLevel());
        EXPECT_NO_THROW(player.upgradeCurrentWeapon());
    }
}

TEST_F(WeaponLevelsTest, EdgeCase_MaxArraySize) {
    EXPECT_EQ(player.weaponLevels.size(), 4u);
    EXPECT_EQ(static_cast<size_t>(WeaponType::COUNT), 4u);
}

