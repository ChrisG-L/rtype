/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** ServerConfigPanel unit tests
*/

#include <gtest/gtest.h>
#include "ui/ServerConfigPanel.hpp"
#include "config/ServerConfigManager.hpp"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

class ServerConfigPanelTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // Use a temp directory for config to avoid polluting real config
        _testConfigDir = fs::temp_directory_path() / "rtype_test_panel";
        fs::create_directories(_testConfigDir);

        // Reset ServerConfigManager to defaults
        auto& manager = config::ServerConfigManager::getInstance();
        manager.setHost("127.0.0.1");
        manager.setTcpPort(4125);
        manager.setUdpPort(4124);
    }

    void TearDown() override
    {
        // Cleanup test directory
        if (fs::exists(_testConfigDir)) {
            fs::remove_all(_testConfigDir);
        }
    }

    fs::path _testConfigDir;
};

// =============================================================================
// Construction Tests
// =============================================================================

TEST_F(ServerConfigPanelTest, ConstructorCreatesPanel)
{
    ui::ServerConfigPanel panel(1920.0f, 1080.0f, "test_font");
    // Panel should be created without throwing
    SUCCEED();
}

TEST_F(ServerConfigPanelTest, ConstructorWithDifferentScreenSizes)
{
    ui::ServerConfigPanel panel1(800.0f, 600.0f, "font1");
    ui::ServerConfigPanel panel2(2560.0f, 1440.0f, "font2");
    ui::ServerConfigPanel panel3(3840.0f, 2160.0f, "font3");
    SUCCEED();
}

// =============================================================================
// Callback Tests
// =============================================================================

TEST_F(ServerConfigPanelTest, SetOnConnectCallback)
{
    ui::ServerConfigPanel panel(1920.0f, 1080.0f, "test_font");

    bool callbackCalled = false;
    panel.setOnConnect([&callbackCalled]() {
        callbackCalled = true;
    });

    // Callback is stored but not called yet
    EXPECT_FALSE(callbackCalled);
}

TEST_F(ServerConfigPanelTest, SetOnCancelCallback)
{
    ui::ServerConfigPanel panel(1920.0f, 1080.0f, "test_font");

    bool callbackCalled = false;
    panel.setOnCancel([&callbackCalled]() {
        callbackCalled = true;
    });

    // Callback is stored but not called yet
    EXPECT_FALSE(callbackCalled);
}

TEST_F(ServerConfigPanelTest, CallbacksCanBeReplaced)
{
    ui::ServerConfigPanel panel(1920.0f, 1080.0f, "test_font");

    int counter = 0;
    panel.setOnConnect([&counter]() { counter = 1; });
    panel.setOnConnect([&counter]() { counter = 2; });

    // Should compile and not crash - new callback replaces old one
    EXPECT_EQ(counter, 0);  // Neither called yet
}

// =============================================================================
// RefreshFromConfig Tests
// =============================================================================

TEST_F(ServerConfigPanelTest, RefreshFromConfigUpdatesValues)
{
    ui::ServerConfigPanel panel(1920.0f, 1080.0f, "test_font");

    auto& manager = config::ServerConfigManager::getInstance();
    manager.setHost("192.168.1.100");
    manager.setTcpPort(5000);
    manager.setUdpPort(5001);

    // Should not throw
    panel.refreshFromConfig();
    SUCCEED();
}

TEST_F(ServerConfigPanelTest, RefreshFromConfigWithPresets)
{
    ui::ServerConfigPanel panel(1920.0f, 1080.0f, "test_font");

    auto& manager = config::ServerConfigManager::getInstance();
    manager.applyFrancePreset();

    panel.refreshFromConfig();

    // Verify manager has France preset values
    EXPECT_EQ(manager.getHost(), "51.254.137.175");
    EXPECT_EQ(manager.getTcpPort(), 4125);
    EXPECT_EQ(manager.getUdpPort(), 4124);
}

TEST_F(ServerConfigPanelTest, RefreshFromConfigWithLocalPreset)
{
    ui::ServerConfigPanel panel(1920.0f, 1080.0f, "test_font");

    auto& manager = config::ServerConfigManager::getInstance();
    manager.applyLocalPreset();

    panel.refreshFromConfig();

    // Verify manager has local preset values
    EXPECT_EQ(manager.getHost(), "127.0.0.1");
    EXPECT_EQ(manager.getTcpPort(), 4125);
    EXPECT_EQ(manager.getUdpPort(), 4124);
}

// =============================================================================
// Update Tests
// =============================================================================

TEST_F(ServerConfigPanelTest, UpdateWithZeroDeltaTime)
{
    ui::ServerConfigPanel panel(1920.0f, 1080.0f, "test_font");
    panel.update(0.0f);
    SUCCEED();
}

TEST_F(ServerConfigPanelTest, UpdateWithPositiveDeltaTime)
{
    ui::ServerConfigPanel panel(1920.0f, 1080.0f, "test_font");
    panel.update(0.016f);  // ~60 FPS
    panel.update(0.033f);  // ~30 FPS
    panel.update(1.0f);    // 1 second
    SUCCEED();
}

TEST_F(ServerConfigPanelTest, UpdateMultipleTimes)
{
    ui::ServerConfigPanel panel(1920.0f, 1080.0f, "test_font");

    for (int i = 0; i < 100; ++i) {
        panel.update(0.016f);
    }
    SUCCEED();
}

// =============================================================================
// HandleEvent Tests (basic - without actual window/input system)
// =============================================================================

TEST_F(ServerConfigPanelTest, HandleEventReturnsTrue)
{
    ui::ServerConfigPanel panel(1920.0f, 1080.0f, "test_font");

    // Create a None event
    events::Event noneEvent = events::None{};
    bool consumed = panel.handleEvent(noneEvent);

    // Panel should consume events when visible
    EXPECT_TRUE(consumed);
}

// =============================================================================
// Move Semantics Tests
// =============================================================================

TEST_F(ServerConfigPanelTest, MoveConstructor)
{
    ui::ServerConfigPanel panel1(1920.0f, 1080.0f, "test_font");
    ui::ServerConfigPanel panel2(std::move(panel1));

    // panel2 should be usable after move
    panel2.update(0.016f);
    SUCCEED();
}

TEST_F(ServerConfigPanelTest, MoveAssignment)
{
    ui::ServerConfigPanel panel1(1920.0f, 1080.0f, "test_font");
    ui::ServerConfigPanel panel2(800.0f, 600.0f, "other_font");

    panel2 = std::move(panel1);

    // panel2 should be usable after move assignment
    panel2.update(0.016f);
    SUCCEED();
}

// =============================================================================
// Integration with ServerConfigManager Tests
// =============================================================================

TEST_F(ServerConfigPanelTest, PanelUsesServerConfigManagerSingleton)
{
    auto& manager = config::ServerConfigManager::getInstance();
    manager.setHost("test.example.com");
    manager.setTcpPort(9999);
    manager.setUdpPort(9998);

    ui::ServerConfigPanel panel(1920.0f, 1080.0f, "test_font");
    panel.refreshFromConfig();

    // Manager should still have the same values
    EXPECT_EQ(manager.getHost(), "test.example.com");
    EXPECT_EQ(manager.getTcpPort(), 9999);
    EXPECT_EQ(manager.getUdpPort(), 9998);
}

TEST_F(ServerConfigPanelTest, MultiplePanelsShareSameConfig)
{
    auto& manager = config::ServerConfigManager::getInstance();
    manager.setHost("shared.server.com");

    ui::ServerConfigPanel panel1(1920.0f, 1080.0f, "font1");
    ui::ServerConfigPanel panel2(1920.0f, 1080.0f, "font2");

    panel1.refreshFromConfig();
    panel2.refreshFromConfig();

    // Both panels should see the same config
    EXPECT_EQ(manager.getHost(), "shared.server.com");
}
