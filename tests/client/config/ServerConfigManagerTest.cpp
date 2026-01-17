/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** Tests unitaires pour ServerConfigManager
*/

#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <thread>
#include <vector>
#include "config/ServerConfigManager.hpp"

/**
 * @brief Suite de tests pour ServerConfigManager
 *
 * ServerConfigManager est un singleton qui gère la configuration
 * du serveur de manière thread-safe avec persistance JSON.
 */
class ServerConfigManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset aux valeurs par défaut
        auto& manager = config::ServerConfigManager::getInstance();
        manager.resetToDefaults();
    }

    void TearDown() override {
        // Nettoyer le fichier de test si créé
        auto& manager = config::ServerConfigManager::getInstance();
        auto configPath = manager.getConfigFilePath();
        if (std::filesystem::exists(configPath)) {
            std::filesystem::remove(configPath);
        }
    }
};

// ============================================================================
// Tests des valeurs par défaut
// ============================================================================

/**
 * @test Vérification des valeurs par défaut
 */
TEST_F(ServerConfigManagerTest, DefaultValues) {
    auto& manager = config::ServerConfigManager::getInstance();

    EXPECT_EQ(manager.getHost(), "127.0.0.1");
    EXPECT_EQ(manager.getTcpPort(), 4125);
    EXPECT_EQ(manager.getUdpPort(), 4124);
    EXPECT_EQ(manager.getVoicePort(), 4126);
}

/**
 * @test Vérification que getConfig retourne une copie valide
 */
TEST_F(ServerConfigManagerTest, GetConfigReturnsCopy) {
    auto& manager = config::ServerConfigManager::getInstance();
    manager.setHost("192.168.1.1");

    config::ServerConfig config = manager.getConfig();
    EXPECT_EQ(config.host, "192.168.1.1");
    EXPECT_EQ(config.tcpPort, 4125);
    EXPECT_EQ(config.udpPort, 4124);
    EXPECT_EQ(config.voicePort, 4126);
}

// ============================================================================
// Tests des setters
// ============================================================================

/**
 * @test Modification du host
 */
TEST_F(ServerConfigManagerTest, SetHost) {
    auto& manager = config::ServerConfigManager::getInstance();

    manager.setHost("51.254.137.175");
    EXPECT_EQ(manager.getHost(), "51.254.137.175");

    manager.setHost("localhost");
    EXPECT_EQ(manager.getHost(), "localhost");

    manager.setHost("myserver.example.com");
    EXPECT_EQ(manager.getHost(), "myserver.example.com");
}

/**
 * @test Modification du port TCP
 */
TEST_F(ServerConfigManagerTest, SetTcpPort) {
    auto& manager = config::ServerConfigManager::getInstance();

    manager.setTcpPort(8080);
    EXPECT_EQ(manager.getTcpPort(), 8080);

    manager.setTcpPort(1);
    EXPECT_EQ(manager.getTcpPort(), 1);

    manager.setTcpPort(65535);
    EXPECT_EQ(manager.getTcpPort(), 65535);
}

/**
 * @test Modification du port UDP
 */
TEST_F(ServerConfigManagerTest, SetUdpPort) {
    auto& manager = config::ServerConfigManager::getInstance();

    manager.setUdpPort(8079);
    EXPECT_EQ(manager.getUdpPort(), 8079);
}

/**
 * @test Modification du port Voice
 */
TEST_F(ServerConfigManagerTest, SetVoicePort) {
    auto& manager = config::ServerConfigManager::getInstance();

    manager.setVoicePort(4200);
    EXPECT_EQ(manager.getVoicePort(), 4200);
}

/**
 * @test Modification de la config complète
 */
TEST_F(ServerConfigManagerTest, SetConfig) {
    auto& manager = config::ServerConfigManager::getInstance();

    config::ServerConfig newConfig{
        .host = "10.0.0.1",
        .tcpPort = 5000,
        .udpPort = 5001,
        .voicePort = 5002
    };

    manager.setConfig(newConfig);

    EXPECT_EQ(manager.getHost(), "10.0.0.1");
    EXPECT_EQ(manager.getTcpPort(), 5000);
    EXPECT_EQ(manager.getUdpPort(), 5001);
    EXPECT_EQ(manager.getVoicePort(), 5002);
}

// ============================================================================
// Tests des presets
// ============================================================================

/**
 * @test Preset France
 */
TEST_F(ServerConfigManagerTest, ApplyFrancePreset) {
    auto& manager = config::ServerConfigManager::getInstance();

    // Partir d'une autre valeur
    manager.setHost("192.168.1.1");
    manager.setTcpPort(9999);

    manager.applyFrancePreset();

    EXPECT_EQ(manager.getHost(), "51.254.137.175");
    EXPECT_EQ(manager.getTcpPort(), 4125);
    EXPECT_EQ(manager.getUdpPort(), 4124);
}

/**
 * @test Preset Local
 */
TEST_F(ServerConfigManagerTest, ApplyLocalPreset) {
    auto& manager = config::ServerConfigManager::getInstance();

    // Partir d'une autre valeur
    manager.setHost("51.254.137.175");
    manager.setTcpPort(9999);

    manager.applyLocalPreset();

    EXPECT_EQ(manager.getHost(), "127.0.0.1");
    EXPECT_EQ(manager.getTcpPort(), 4125);
    EXPECT_EQ(manager.getUdpPort(), 4124);
}

// ============================================================================
// Tests de reset
// ============================================================================

/**
 * @test Reset aux valeurs par défaut
 */
TEST_F(ServerConfigManagerTest, ResetToDefaults) {
    auto& manager = config::ServerConfigManager::getInstance();

    // Modifier toutes les valeurs
    manager.setHost("custom.server.com");
    manager.setTcpPort(9000);
    manager.setUdpPort(9001);
    manager.setVoicePort(9002);

    // Reset
    manager.resetToDefaults();

    EXPECT_EQ(manager.getHost(), "127.0.0.1");
    EXPECT_EQ(manager.getTcpPort(), 4125);
    EXPECT_EQ(manager.getUdpPort(), 4124);
    EXPECT_EQ(manager.getVoicePort(), 4126);
}

// ============================================================================
// Tests du Singleton
// ============================================================================

/**
 * @test Vérification que c'est bien un singleton
 */
TEST_F(ServerConfigManagerTest, SingletonBehavior) {
    auto& manager1 = config::ServerConfigManager::getInstance();
    auto& manager2 = config::ServerConfigManager::getInstance();

    // Mêmes adresses = même instance
    EXPECT_EQ(&manager1, &manager2);

    // Modification via une référence visible par l'autre
    manager1.setHost("test.server");
    EXPECT_EQ(manager2.getHost(), "test.server");
}

// ============================================================================
// Tests de thread safety
// ============================================================================

/**
 * @test Accès concurrent en lecture
 */
TEST_F(ServerConfigManagerTest, ConcurrentReads) {
    auto& manager = config::ServerConfigManager::getInstance();
    manager.setHost("concurrent.test");
    manager.setTcpPort(5555);

    std::vector<std::thread> threads;
    std::atomic<int> successCount{0};

    // Lancer 10 threads qui lisent en parallèle
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&manager, &successCount]() {
            for (int j = 0; j < 100; ++j) {
                std::string host = manager.getHost();
                uint16_t port = manager.getTcpPort();
                if (host == "concurrent.test" && port == 5555) {
                    ++successCount;
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(successCount.load(), 1000);
}

/**
 * @test Accès concurrent en écriture
 */
TEST_F(ServerConfigManagerTest, ConcurrentWrites) {
    auto& manager = config::ServerConfigManager::getInstance();

    std::vector<std::thread> threads;

    // Thread 1 : applique preset France en boucle
    threads.emplace_back([&manager]() {
        for (int i = 0; i < 50; ++i) {
            manager.setHost("51.254.137.175");
            manager.setTcpPort(4125);
        }
    });

    // Thread 2 : applique preset Local en boucle
    threads.emplace_back([&manager]() {
        for (int i = 0; i < 50; ++i) {
            manager.setHost("127.0.0.1");
            manager.setTcpPort(4125);
        }
    });

    for (auto& t : threads) {
        t.join();
    }

    // Le host doit être soit France soit Local (pas corrompu)
    std::string finalHost = manager.getHost();
    EXPECT_TRUE(finalHost == "51.254.137.175" || finalHost == "127.0.0.1");
}

/**
 * @test getConfig() retourne une copie thread-safe
 */
TEST_F(ServerConfigManagerTest, GetConfigThreadSafe) {
    auto& manager = config::ServerConfigManager::getInstance();
    manager.setHost("initial.host");

    std::vector<std::thread> threads;
    std::atomic<int> validCopies{0};

    // Thread lecteur qui obtient des copies
    threads.emplace_back([&manager, &validCopies]() {
        for (int i = 0; i < 100; ++i) {
            config::ServerConfig config = manager.getConfig();
            // La copie doit être cohérente (pas de corruption)
            if (!config.host.empty() && config.tcpPort > 0) {
                ++validCopies;
            }
        }
    });

    // Thread écrivain
    threads.emplace_back([&manager]() {
        for (int i = 0; i < 100; ++i) {
            manager.setHost("changed.host." + std::to_string(i));
            manager.setTcpPort(static_cast<uint16_t>(4000 + i));
        }
    });

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(validCopies.load(), 100);
}

// ============================================================================
// Tests de persistance (fichier config)
// ============================================================================

/**
 * @test Sauvegarde et chargement de la config
 */
TEST_F(ServerConfigManagerTest, SaveAndLoad) {
    auto& manager = config::ServerConfigManager::getInstance();

    // Configurer des valeurs custom
    manager.setHost("persistent.server.com");
    manager.setTcpPort(7777);
    manager.setUdpPort(7778);
    manager.setVoicePort(7779);

    // Sauvegarder
    EXPECT_TRUE(manager.save());

    // Vérifier que le fichier existe
    EXPECT_TRUE(std::filesystem::exists(manager.getConfigFilePath()));

    // Reset pour simuler un redémarrage
    manager.resetToDefaults();
    EXPECT_EQ(manager.getHost(), "127.0.0.1");

    // Recharger
    EXPECT_TRUE(manager.load());

    // Vérifier les valeurs restaurées
    EXPECT_EQ(manager.getHost(), "persistent.server.com");
    EXPECT_EQ(manager.getTcpPort(), 7777);
    EXPECT_EQ(manager.getUdpPort(), 7778);
    EXPECT_EQ(manager.getVoicePort(), 7779);
}

/**
 * @test Chemin du fichier de config
 */
TEST_F(ServerConfigManagerTest, ConfigFilePath) {
    auto& manager = config::ServerConfigManager::getInstance();
    auto path = manager.getConfigFilePath();

    // Le chemin doit contenir "rtype" et finir par ".json"
    EXPECT_TRUE(path.string().find("rtype") != std::string::npos);
    EXPECT_EQ(path.extension(), ".json");
}

/**
 * @test Load avec fichier inexistant retourne false
 */
TEST_F(ServerConfigManagerTest, LoadNonExistentFile) {
    auto& manager = config::ServerConfigManager::getInstance();

    // Supprimer le fichier s'il existe
    auto configPath = manager.getConfigFilePath();
    if (std::filesystem::exists(configPath)) {
        std::filesystem::remove(configPath);
    }

    // Reset et vérifier que load retourne false
    manager.resetToDefaults();
    EXPECT_FALSE(manager.load());

    // Les valeurs par défaut doivent être préservées
    EXPECT_EQ(manager.getHost(), "127.0.0.1");
}

// ============================================================================
// Tests de validation des données
// ============================================================================

/**
 * @test Host vide est accepté (pas de validation)
 */
TEST_F(ServerConfigManagerTest, EmptyHostAccepted) {
    auto& manager = config::ServerConfigManager::getInstance();

    manager.setHost("");
    EXPECT_EQ(manager.getHost(), "");
}

/**
 * @test Ports limites (min/max)
 */
TEST_F(ServerConfigManagerTest, PortBoundaries) {
    auto& manager = config::ServerConfigManager::getInstance();

    // Port minimum
    manager.setTcpPort(1);
    EXPECT_EQ(manager.getTcpPort(), 1);

    // Port maximum
    manager.setTcpPort(65535);
    EXPECT_EQ(manager.getTcpPort(), 65535);

    // Port zéro (techniquement invalide mais accepté)
    manager.setTcpPort(0);
    EXPECT_EQ(manager.getTcpPort(), 0);
}
