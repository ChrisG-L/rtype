/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** Tests unitaires pour AccessibilityConfig
*/

#include <gtest/gtest.h>
#include "accessibility/AccessibilityConfig.hpp"

/**
 * @brief Suite de tests pour AccessibilityConfig
 *
 * Teste la gestion des raccourcis clavier configurables,
 * les modes daltonien, et les paramètres d'accessibilité.
 */
class AccessibilityConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset to default bindings before each test
        auto& config = accessibility::AccessibilityConfig::getInstance();
        config.resetKeyBindings();
        config.setColorBlindMode(accessibility::ColorBlindMode::None);
        config.setGameSpeedMultiplier(1.0f);
        config.setShipSkin(1);
    }
    void TearDown() override {}
};

// ============================================================================
// GameAction Enum Tests
// ============================================================================

/**
 * @test Vérifier que toutes les GameActions sont définies
 */
TEST_F(AccessibilityConfigTest, GameActionEnumCount) {
    // 13 actions: MoveUp, MoveDown, MoveLeft, MoveRight, Shoot, Pause, PushToTalk,
    //             WeaponPrev, WeaponNext, OpenChat, ExpandChat, ForceToggle, ToggleControls
    EXPECT_EQ(static_cast<size_t>(accessibility::GameAction::ActionCount), 13);
}

// ============================================================================
// Key Bindings Tests
// ============================================================================

/**
 * @test Vérifier les bindings par défaut pour le mouvement
 */
TEST_F(AccessibilityConfigTest, DefaultMovementBindings) {
    auto& config = accessibility::AccessibilityConfig::getInstance();

    // MoveUp: Up / Z
    EXPECT_EQ(config.getPrimaryKey(accessibility::GameAction::MoveUp), events::Key::Up);
    EXPECT_EQ(config.getSecondaryKey(accessibility::GameAction::MoveUp), events::Key::Z);

    // MoveDown: Down / S
    EXPECT_EQ(config.getPrimaryKey(accessibility::GameAction::MoveDown), events::Key::Down);
    EXPECT_EQ(config.getSecondaryKey(accessibility::GameAction::MoveDown), events::Key::S);

    // MoveLeft: Left / A (WASD layout)
    EXPECT_EQ(config.getPrimaryKey(accessibility::GameAction::MoveLeft), events::Key::Left);
    EXPECT_EQ(config.getSecondaryKey(accessibility::GameAction::MoveLeft), events::Key::A);

    // MoveRight: Right / D
    EXPECT_EQ(config.getPrimaryKey(accessibility::GameAction::MoveRight), events::Key::Right);
    EXPECT_EQ(config.getSecondaryKey(accessibility::GameAction::MoveRight), events::Key::D);
}

/**
 * @test Vérifier les bindings par défaut pour les actions
 */
TEST_F(AccessibilityConfigTest, DefaultActionBindings) {
    auto& config = accessibility::AccessibilityConfig::getInstance();

    // Shoot: Space / Enter
    EXPECT_EQ(config.getPrimaryKey(accessibility::GameAction::Shoot), events::Key::Space);
    EXPECT_EQ(config.getSecondaryKey(accessibility::GameAction::Shoot), events::Key::Enter);

    // Pause: Escape / P
    EXPECT_EQ(config.getPrimaryKey(accessibility::GameAction::Pause), events::Key::Escape);
    EXPECT_EQ(config.getSecondaryKey(accessibility::GameAction::Pause), events::Key::P);
}

/**
 * @test Vérifier le binding par défaut pour Push-to-Talk
 */
TEST_F(AccessibilityConfigTest, DefaultPushToTalkBinding) {
    auto& config = accessibility::AccessibilityConfig::getInstance();

    // PushToTalk: V / Unknown (no secondary)
    EXPECT_EQ(config.getPrimaryKey(accessibility::GameAction::PushToTalk), events::Key::V);
    EXPECT_EQ(config.getSecondaryKey(accessibility::GameAction::PushToTalk), events::Key::Unknown);
}

/**
 * @test Modifier un binding de touche
 */
TEST_F(AccessibilityConfigTest, SetKeyBinding) {
    auto& config = accessibility::AccessibilityConfig::getInstance();

    config.setKeyBinding(accessibility::GameAction::PushToTalk, events::Key::M, events::Key::N);

    EXPECT_EQ(config.getPrimaryKey(accessibility::GameAction::PushToTalk), events::Key::M);
    EXPECT_EQ(config.getSecondaryKey(accessibility::GameAction::PushToTalk), events::Key::N);
}

/**
 * @test isActionKey avec touche primaire
 */
TEST_F(AccessibilityConfigTest, IsActionKeyPrimary) {
    auto& config = accessibility::AccessibilityConfig::getInstance();

    EXPECT_TRUE(config.isActionKey(accessibility::GameAction::PushToTalk, events::Key::V));
    EXPECT_FALSE(config.isActionKey(accessibility::GameAction::PushToTalk, events::Key::X));
}

/**
 * @test isActionKey avec touche secondaire
 */
TEST_F(AccessibilityConfigTest, IsActionKeySecondary) {
    auto& config = accessibility::AccessibilityConfig::getInstance();

    // MoveUp a Z comme touche secondaire
    EXPECT_TRUE(config.isActionKey(accessibility::GameAction::MoveUp, events::Key::Z));
    EXPECT_TRUE(config.isActionKey(accessibility::GameAction::MoveUp, events::Key::Up));
}

/**
 * @test isActionKey avec action invalide (ActionCount)
 */
TEST_F(AccessibilityConfigTest, IsActionKeyInvalidAction) {
    auto& config = accessibility::AccessibilityConfig::getInstance();

    // ActionCount n'est pas une action valide
    EXPECT_FALSE(config.isActionKey(accessibility::GameAction::ActionCount, events::Key::V));
}

/**
 * @test Reset des bindings aux valeurs par défaut
 */
TEST_F(AccessibilityConfigTest, ResetKeyBindings) {
    auto& config = accessibility::AccessibilityConfig::getInstance();

    // Modifier un binding
    config.setKeyBinding(accessibility::GameAction::Shoot, events::Key::X, events::Key::Y);
    EXPECT_EQ(config.getPrimaryKey(accessibility::GameAction::Shoot), events::Key::X);

    // Reset
    config.resetKeyBindings();

    // Vérifier que c'est revenu aux valeurs par défaut
    EXPECT_EQ(config.getPrimaryKey(accessibility::GameAction::Shoot), events::Key::Space);
    EXPECT_EQ(config.getSecondaryKey(accessibility::GameAction::Shoot), events::Key::Enter);
}

// ============================================================================
// Key to String Conversion Tests
// ============================================================================

/**
 * @test Conversion Key vers String
 */
TEST_F(AccessibilityConfigTest, KeyToString) {
    EXPECT_EQ(accessibility::AccessibilityConfig::keyToString(events::Key::V), "V");
    EXPECT_EQ(accessibility::AccessibilityConfig::keyToString(events::Key::Space), "Space");
    EXPECT_EQ(accessibility::AccessibilityConfig::keyToString(events::Key::Up), "Up");
    EXPECT_EQ(accessibility::AccessibilityConfig::keyToString(events::Key::Enter), "Enter");
    EXPECT_EQ(accessibility::AccessibilityConfig::keyToString(events::Key::Unknown), "Unknown");
}

/**
 * @test Conversion String vers Key
 */
TEST_F(AccessibilityConfigTest, StringToKey) {
    EXPECT_EQ(accessibility::AccessibilityConfig::stringToKey("V"), events::Key::V);
    EXPECT_EQ(accessibility::AccessibilityConfig::stringToKey("Space"), events::Key::Space);
    EXPECT_EQ(accessibility::AccessibilityConfig::stringToKey("Up"), events::Key::Up);
    EXPECT_EQ(accessibility::AccessibilityConfig::stringToKey("Enter"), events::Key::Enter);
    EXPECT_EQ(accessibility::AccessibilityConfig::stringToKey("InvalidKey"), events::Key::Unknown);
}

// ============================================================================
// ColorBlind Mode Tests
// ============================================================================

/**
 * @test Mode daltonien par défaut
 */
TEST_F(AccessibilityConfigTest, DefaultColorBlindMode) {
    auto& config = accessibility::AccessibilityConfig::getInstance();
    EXPECT_EQ(config.getColorBlindMode(), accessibility::ColorBlindMode::None);
}

/**
 * @test Changer le mode daltonien
 */
TEST_F(AccessibilityConfigTest, SetColorBlindMode) {
    auto& config = accessibility::AccessibilityConfig::getInstance();

    config.setColorBlindMode(accessibility::ColorBlindMode::Protanopia);
    EXPECT_EQ(config.getColorBlindMode(), accessibility::ColorBlindMode::Protanopia);

    config.setColorBlindMode(accessibility::ColorBlindMode::Deuteranopia);
    EXPECT_EQ(config.getColorBlindMode(), accessibility::ColorBlindMode::Deuteranopia);

    config.setColorBlindMode(accessibility::ColorBlindMode::Tritanopia);
    EXPECT_EQ(config.getColorBlindMode(), accessibility::ColorBlindMode::Tritanopia);

    config.setColorBlindMode(accessibility::ColorBlindMode::HighContrast);
    EXPECT_EQ(config.getColorBlindMode(), accessibility::ColorBlindMode::HighContrast);
}

/**
 * @test Conversion ColorBlindMode vers String
 */
TEST_F(AccessibilityConfigTest, ColorBlindModeToString) {
    EXPECT_EQ(accessibility::AccessibilityConfig::colorBlindModeToString(accessibility::ColorBlindMode::None), "none");
    EXPECT_EQ(accessibility::AccessibilityConfig::colorBlindModeToString(accessibility::ColorBlindMode::Protanopia), "protanopia");
    EXPECT_EQ(accessibility::AccessibilityConfig::colorBlindModeToString(accessibility::ColorBlindMode::Deuteranopia), "deuteranopia");
    EXPECT_EQ(accessibility::AccessibilityConfig::colorBlindModeToString(accessibility::ColorBlindMode::Tritanopia), "tritanopia");
    EXPECT_EQ(accessibility::AccessibilityConfig::colorBlindModeToString(accessibility::ColorBlindMode::HighContrast), "high_contrast");
}

/**
 * @test Conversion String vers ColorBlindMode
 */
TEST_F(AccessibilityConfigTest, StringToColorBlindMode) {
    EXPECT_EQ(accessibility::AccessibilityConfig::stringToColorBlindMode("none"), accessibility::ColorBlindMode::None);
    EXPECT_EQ(accessibility::AccessibilityConfig::stringToColorBlindMode("protanopia"), accessibility::ColorBlindMode::Protanopia);
    EXPECT_EQ(accessibility::AccessibilityConfig::stringToColorBlindMode("deuteranopia"), accessibility::ColorBlindMode::Deuteranopia);
    EXPECT_EQ(accessibility::AccessibilityConfig::stringToColorBlindMode("tritanopia"), accessibility::ColorBlindMode::Tritanopia);
    EXPECT_EQ(accessibility::AccessibilityConfig::stringToColorBlindMode("high_contrast"), accessibility::ColorBlindMode::HighContrast);
    EXPECT_EQ(accessibility::AccessibilityConfig::stringToColorBlindMode("invalid"), accessibility::ColorBlindMode::None);
}

// ============================================================================
// Color Palette Tests
// ============================================================================

/**
 * @test Les couleurs changent selon le mode daltonien
 */
TEST_F(AccessibilityConfigTest, ColorPaletteChangesWithMode) {
    auto& config = accessibility::AccessibilityConfig::getInstance();

    // Couleurs par défaut (mode None) - player color is {0, 150, 255, 255}
    auto defaultPlayerColor = config.getPlayerColor();
    EXPECT_EQ(defaultPlayerColor.r, 0);
    EXPECT_EQ(defaultPlayerColor.g, 150);

    // Changer en mode HighContrast - player color becomes {0, 255, 255, 255}
    config.setColorBlindMode(accessibility::ColorBlindMode::HighContrast);
    auto highContrastPlayerColor = config.getPlayerColor();

    // Les couleurs doivent être différentes (g change: 150 -> 255)
    EXPECT_NE(defaultPlayerColor.g, highContrastPlayerColor.g);
    EXPECT_EQ(highContrastPlayerColor.g, 255);
}

/**
 * @test Toutes les couleurs sont accessibles
 */
TEST_F(AccessibilityConfigTest, AllColorsAccessible) {
    auto& config = accessibility::AccessibilityConfig::getInstance();

    // Vérifier que toutes les méthodes retournent des couleurs valides
    auto playerColor = config.getPlayerColor();
    auto otherPlayerColor = config.getOtherPlayerColor();
    auto enemyColor = config.getEnemyColor();
    auto playerMissileColor = config.getPlayerMissileColor();
    auto enemyMissileColor = config.getEnemyMissileColor();
    auto healthHigh = config.getHealthHighColor();
    auto healthMedium = config.getHealthMediumColor();
    auto healthLow = config.getHealthLowColor();

    // Toutes les couleurs doivent avoir une composante alpha
    EXPECT_EQ(playerColor.a, 255);
    EXPECT_EQ(otherPlayerColor.a, 255);
    EXPECT_EQ(enemyColor.a, 255);
    EXPECT_EQ(playerMissileColor.a, 255);
    EXPECT_EQ(enemyMissileColor.a, 255);
    EXPECT_EQ(healthHigh.a, 255);
    EXPECT_EQ(healthMedium.a, 255);
    EXPECT_EQ(healthLow.a, 255);
}

// ============================================================================
// Game Speed Tests
// ============================================================================

/**
 * @test Vitesse de jeu par défaut
 */
TEST_F(AccessibilityConfigTest, DefaultGameSpeed) {
    auto& config = accessibility::AccessibilityConfig::getInstance();
    EXPECT_FLOAT_EQ(config.getGameSpeedMultiplier(), 1.0f);
}

/**
 * @test Modifier la vitesse de jeu
 */
TEST_F(AccessibilityConfigTest, SetGameSpeed) {
    auto& config = accessibility::AccessibilityConfig::getInstance();

    config.setGameSpeedMultiplier(0.75f);
    EXPECT_FLOAT_EQ(config.getGameSpeedMultiplier(), 0.75f);

    config.setGameSpeedMultiplier(1.5f);
    EXPECT_FLOAT_EQ(config.getGameSpeedMultiplier(), 1.5f);
}

/**
 * @test La vitesse de jeu est clampée entre 0.5 et 2.0
 */
TEST_F(AccessibilityConfigTest, GameSpeedClamping) {
    auto& config = accessibility::AccessibilityConfig::getInstance();

    // Trop bas
    config.setGameSpeedMultiplier(0.1f);
    EXPECT_FLOAT_EQ(config.getGameSpeedMultiplier(), 0.5f);

    // Trop haut
    config.setGameSpeedMultiplier(5.0f);
    EXPECT_FLOAT_EQ(config.getGameSpeedMultiplier(), 2.0f);

    // Dans les limites
    config.setGameSpeedMultiplier(1.25f);
    EXPECT_FLOAT_EQ(config.getGameSpeedMultiplier(), 1.25f);
}

// ============================================================================
// Ship Skin Tests
// ============================================================================

/**
 * @test Skin de vaisseau par défaut
 */
TEST_F(AccessibilityConfigTest, DefaultShipSkin) {
    auto& config = accessibility::AccessibilityConfig::getInstance();
    EXPECT_EQ(config.getShipSkin(), 1);
}

/**
 * @test Modifier le skin de vaisseau
 */
TEST_F(AccessibilityConfigTest, SetShipSkin) {
    auto& config = accessibility::AccessibilityConfig::getInstance();

    config.setShipSkin(3);
    EXPECT_EQ(config.getShipSkin(), 3);

    config.setShipSkin(6);
    EXPECT_EQ(config.getShipSkin(), 6);
}

/**
 * @test Le skin de vaisseau est clampé entre 1 et 6
 */
TEST_F(AccessibilityConfigTest, ShipSkinClamping) {
    auto& config = accessibility::AccessibilityConfig::getInstance();

    // Trop bas
    config.setShipSkin(0);
    EXPECT_EQ(config.getShipSkin(), 1);

    // Trop haut
    config.setShipSkin(10);
    EXPECT_EQ(config.getShipSkin(), 6);

    // Dans les limites
    config.setShipSkin(4);
    EXPECT_EQ(config.getShipSkin(), 4);
}

// ============================================================================
// Singleton Tests
// ============================================================================

/**
 * @test Le singleton retourne toujours la même instance
 */
TEST_F(AccessibilityConfigTest, SingletonSameInstance) {
    auto& config1 = accessibility::AccessibilityConfig::getInstance();
    auto& config2 = accessibility::AccessibilityConfig::getInstance();

    // Modifier via config1
    config1.setShipSkin(5);

    // Vérifier via config2
    EXPECT_EQ(config2.getShipSkin(), 5);

    // Les adresses doivent être identiques
    EXPECT_EQ(&config1, &config2);
}

// ============================================================================
// Edge Cases
// ============================================================================

/**
 * @test Binding avec Unknown comme touche secondaire
 */
TEST_F(AccessibilityConfigTest, BindingWithUnknownSecondary) {
    auto& config = accessibility::AccessibilityConfig::getInstance();

    // PushToTalk n'a pas de touche secondaire par défaut
    EXPECT_EQ(config.getSecondaryKey(accessibility::GameAction::PushToTalk), events::Key::Unknown);

    // La touche primaire (V) doit matcher
    EXPECT_TRUE(config.isActionKey(accessibility::GameAction::PushToTalk, events::Key::V));

    // Une touche non assignée ne doit pas matcher
    EXPECT_FALSE(config.isActionKey(accessibility::GameAction::PushToTalk, events::Key::X));
}

/**
 * @test getPrimaryKey avec action invalide
 */
TEST_F(AccessibilityConfigTest, GetKeyInvalidAction) {
    auto& config = accessibility::AccessibilityConfig::getInstance();

    // ActionCount est hors limites
    EXPECT_EQ(config.getPrimaryKey(accessibility::GameAction::ActionCount), events::Key::Unknown);
    EXPECT_EQ(config.getSecondaryKey(accessibility::GameAction::ActionCount), events::Key::Unknown);
}

/**
 * @test setKeyBinding avec action invalide (ne doit pas crash)
 */
TEST_F(AccessibilityConfigTest, SetBindingInvalidAction) {
    auto& config = accessibility::AccessibilityConfig::getInstance();

    // Ne doit pas crash, mais ne fait rien
    config.setKeyBinding(accessibility::GameAction::ActionCount, events::Key::X, events::Key::Y);

    // Vérifier que les autres bindings n'ont pas changé
    EXPECT_EQ(config.getPrimaryKey(accessibility::GameAction::MoveUp), events::Key::Up);
}
