/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** Tests unitaires pour le système Signal/Slot
*/

#include <gtest/gtest.h>
#include "events/Signal.hpp"
#include <string>
#include <vector>

/**
 * @brief Suite de tests pour le système Signal/Slot
 *
 * Signal est un système d'événements générique permettant
 * de connecter des callbacks (slots) à des signaux et de les
 * déclencher avec des arguments typés.
 */
class SignalTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// ============================================================================
// Tests de Base - Signal sans arguments
// ============================================================================

/**
 * @test Signal sans arguments avec un slot
 */
TEST_F(SignalTest, SignalNoArgsOneSlot) {
    Signal<> signal;
    bool called = false;

    signal.connect([&called]() {
        called = true;
    });

    EXPECT_FALSE(called);
    signal.emit();
    EXPECT_TRUE(called);
}

/**
 * @test Signal sans arguments avec plusieurs slots
 */
TEST_F(SignalTest, SignalNoArgsMultipleSlots) {
    Signal<> signal;
    int callCount = 0;

    signal.connect([&callCount]() { callCount++; });
    signal.connect([&callCount]() { callCount++; });
    signal.connect([&callCount]() { callCount++; });

    signal.emit();
    EXPECT_EQ(callCount, 3);
}

/**
 * @test Signal sans slots connectés
 */
TEST_F(SignalTest, SignalNoSlotsConnected) {
    Signal<> signal;
    // Ne devrait pas crash
    EXPECT_NO_THROW({
        signal.emit();
    });
}

/**
 * @test Multiple emit sur le même signal
 */
TEST_F(SignalTest, MultipleEmits) {
    Signal<> signal;
    int callCount = 0;

    signal.connect([&callCount]() { callCount++; });

    signal.emit();
    signal.emit();
    signal.emit();

    EXPECT_EQ(callCount, 3);
}

// ============================================================================
// Tests avec Un Argument
// ============================================================================

/**
 * @test Signal avec un argument int
 */
TEST_F(SignalTest, SignalOneIntArg) {
    Signal<int> signal;
    int receivedValue = 0;

    signal.connect([&receivedValue](int value) {
        receivedValue = value;
    });

    signal.emit(42);
    EXPECT_EQ(receivedValue, 42);
}

/**
 * @test Signal avec un argument string
 */
TEST_F(SignalTest, SignalOneStringArg) {
    Signal<std::string> signal;
    std::string receivedValue;

    signal.connect([&receivedValue](const std::string& value) {
        receivedValue = value;
    });

    signal.emit("Hello Signal!");
    EXPECT_EQ(receivedValue, "Hello Signal!");
}

/**
 * @test Signal avec un argument float
 */
TEST_F(SignalTest, SignalOneFloatArg) {
    Signal<float> signal;
    float receivedValue = 0.0f;

    signal.connect([&receivedValue](float value) {
        receivedValue = value;
    });

    signal.emit(3.14f);
    EXPECT_FLOAT_EQ(receivedValue, 3.14f);
}

// ============================================================================
// Tests avec Plusieurs Arguments
// ============================================================================

/**
 * @test Signal avec deux arguments
 */
TEST_F(SignalTest, SignalTwoArgs) {
    Signal<int, std::string> signal;
    int receivedInt = 0;
    std::string receivedString;

    signal.connect([&receivedInt, &receivedString](int i, const std::string& s) {
        receivedInt = i;
        receivedString = s;
    });

    signal.emit(100, "test");
    EXPECT_EQ(receivedInt, 100);
    EXPECT_EQ(receivedString, "test");
}

/**
 * @test Signal avec trois arguments
 */
TEST_F(SignalTest, SignalThreeArgs) {
    Signal<int, float, bool> signal;
    int receivedInt = 0;
    float receivedFloat = 0.0f;
    bool receivedBool = false;

    signal.connect([&](int i, float f, bool b) {
        receivedInt = i;
        receivedFloat = f;
        receivedBool = b;
    });

    signal.emit(42, 2.5f, true);
    EXPECT_EQ(receivedInt, 42);
    EXPECT_FLOAT_EQ(receivedFloat, 2.5f);
    EXPECT_TRUE(receivedBool);
}

// ============================================================================
// Tests avec Plusieurs Slots
// ============================================================================

/**
 * @test Plusieurs slots reçoivent le même argument
 */
TEST_F(SignalTest, MultipleSlotsReceiveSameArg) {
    Signal<int> signal;
    std::vector<int> receivedValues;

    signal.connect([&receivedValues](int v) { receivedValues.push_back(v); });
    signal.connect([&receivedValues](int v) { receivedValues.push_back(v * 2); });
    signal.connect([&receivedValues](int v) { receivedValues.push_back(v * 3); });

    signal.emit(10);

    ASSERT_EQ(receivedValues.size(), 3u);
    EXPECT_EQ(receivedValues[0], 10);
    EXPECT_EQ(receivedValues[1], 20);
    EXPECT_EQ(receivedValues[2], 30);
}

/**
 * @test Ordre d'exécution des slots
 */
TEST_F(SignalTest, SlotExecutionOrder) {
    Signal<> signal;
    std::vector<int> order;

    signal.connect([&order]() { order.push_back(1); });
    signal.connect([&order]() { order.push_back(2); });
    signal.connect([&order]() { order.push_back(3); });

    signal.emit();

    ASSERT_EQ(order.size(), 3u);
    EXPECT_EQ(order[0], 1);
    EXPECT_EQ(order[1], 2);
    EXPECT_EQ(order[2], 3);
}

// ============================================================================
// Tests de Cas d'Usage Réels
// ============================================================================

/**
 * @test Signal onClick simulant un bouton
 */
TEST_F(SignalTest, OnClickButtonSimulation) {
    Signal<std::string, int> onClick;

    std::string clickedButton;
    int clickX = 0;

    onClick.connect([&clickedButton, &clickX](const std::string& name, int x) {
        clickedButton = name;
        clickX = x;
    });

    onClick.emit("MainButton", 150);

    EXPECT_EQ(clickedButton, "MainButton");
    EXPECT_EQ(clickX, 150);
}

/**
 * @test Signal pour événement de jeu
 */
TEST_F(SignalTest, GameEventSimulation) {
    Signal<std::string, float, float> onPlayerMove;

    std::string playerId;
    float newX = 0.0f;
    float newY = 0.0f;

    onPlayerMove.connect([&](const std::string& id, float x, float y) {
        playerId = id;
        newX = x;
        newY = y;
    });

    onPlayerMove.emit("player_001", 100.5f, 200.75f);

    EXPECT_EQ(playerId, "player_001");
    EXPECT_FLOAT_EQ(newX, 100.5f);
    EXPECT_FLOAT_EQ(newY, 200.75f);
}

/**
 * @test Signal avec callback qui modifie l'état
 */
TEST_F(SignalTest, StateModificationCallback) {
    Signal<int> onScoreChange;

    int totalScore = 0;

    // Premier slot : ajoute au score
    onScoreChange.connect([&totalScore](int points) {
        totalScore += points;
    });

    // Second slot : bonus si score > 100
    onScoreChange.connect([&totalScore]([[maybe_unused]] int points) {
        if (totalScore > 100) {
            totalScore += 10;  // Bonus
        }
    });

    onScoreChange.emit(50);
    EXPECT_EQ(totalScore, 50);

    onScoreChange.emit(60);  // total = 110, puis +10 bonus
    EXPECT_EQ(totalScore, 120);
}

// ============================================================================
// Tests de Robustesse
// ============================================================================

/**
 * @test Signal avec lambda capturant par valeur
 */
TEST_F(SignalTest, LambdaCaptureByValue) {
    Signal<int> signal;
    int capturedValue = 100;

    signal.connect([capturedValue]([[maybe_unused]] int v) {
        // capturedValue est une copie
        EXPECT_EQ(capturedValue, 100);
    });

    capturedValue = 200;  // Modifier l'original
    signal.emit(0);  // La lambda utilise toujours 100
}

/**
 * @test Signal avec lambda capturant par référence
 */
TEST_F(SignalTest, LambdaCaptureByReference) {
    Signal<int> signal;
    int value = 0;

    signal.connect([&value](int v) {
        value = v;
    });

    signal.emit(42);
    EXPECT_EQ(value, 42);

    signal.emit(100);
    EXPECT_EQ(value, 100);
}

/**
 * @test Emit avec valeurs par défaut de types
 */
TEST_F(SignalTest, EmitWithDefaultTypeValues) {
    Signal<int, std::string, bool> signal;
    int receivedInt = -1;
    std::string receivedString = "initial";
    bool receivedBool = true;

    signal.connect([&](int i, const std::string& s, bool b) {
        receivedInt = i;
        receivedString = s;
        receivedBool = b;
    });

    signal.emit(0, "", false);

    EXPECT_EQ(receivedInt, 0);
    EXPECT_EQ(receivedString, "");
    EXPECT_FALSE(receivedBool);
}

// ============================================================================
// Tests de Performance (basiques)
// ============================================================================

/**
 * @test Beaucoup de slots
 */
TEST_F(SignalTest, ManySlots) {
    Signal<int> signal;
    int callCount = 0;

    for (int i = 0; i < 100; i++) {
        signal.connect([&callCount](int) { callCount++; });
    }

    signal.emit(1);
    EXPECT_EQ(callCount, 100);
}

/**
 * @test Beaucoup d'émissions
 */
TEST_F(SignalTest, ManyEmits) {
    Signal<> signal;
    int callCount = 0;

    signal.connect([&callCount]() { callCount++; });

    for (int i = 0; i < 1000; i++) {
        signal.emit();
    }

    EXPECT_EQ(callCount, 1000);
}
