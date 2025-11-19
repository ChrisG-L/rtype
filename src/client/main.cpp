/*
** EPITECH PROJECT, 2025
** R-Type
** File description:
** main - Point d'entrée du client
*/

#include "core/RTypeGame.hpp"
#include <iostream>
#include <cstring>

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [options]\n"
              << "Options:\n"
              << "  -h, --help           Affiche cette aide\n"
              << "  -s, --server HOST    Adresse du serveur (défaut: localhost)\n"
              << "  -p, --port PORT      Port du serveur (défaut: 4242)\n"
              << "  -n, --name NAME      Nom du joueur (défaut: Player)\n"
              << "  -f, --fullscreen     Mode plein écran\n"
              << "  -w, --width WIDTH    Largeur de la fenêtre (défaut: 1920)\n"
              << "  --height HEIGHT      Hauteur de la fenêtre (défaut: 1080)\n"
              << "  --offline            Mode hors-ligne (sans serveur)\n"
              << std::endl;
}

int main(int argc, char* argv[]) {
    // Configuration par défaut
    rtype::core::GameConfig config;
    config.title = "R-Type";
    config.width = 1920;
    config.height = 1080;
    config.fullscreen = false;
    config.vsync = true;
    config.tickRate = 60.0f;

    std::string serverHost = "localhost";
    std::uint16_t serverPort = 4242;
    std::string playerName = "Player";
    bool offline = false;

    // Parse les arguments
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "-h") == 0 || std::strcmp(argv[i], "--help") == 0) {
            printUsage(argv[0]);
            return 0;
        }
        else if (std::strcmp(argv[i], "-s") == 0 || std::strcmp(argv[i], "--server") == 0) {
            if (i + 1 < argc) {
                serverHost = argv[++i];
            }
        }
        else if (std::strcmp(argv[i], "-p") == 0 || std::strcmp(argv[i], "--port") == 0) {
            if (i + 1 < argc) {
                try {
                    int port = std::stoi(argv[++i]);
                    if (port < 1 || port > 65535) {
                        std::cerr << "Erreur: Port doit être entre 1 et 65535" << std::endl;
                        return 1;
                    }
                    serverPort = static_cast<std::uint16_t>(port);
                } catch (const std::exception&) {
                    std::cerr << "Erreur: Port invalide" << std::endl;
                    return 1;
                }
            }
        }
        else if (std::strcmp(argv[i], "-n") == 0 || std::strcmp(argv[i], "--name") == 0) {
            if (i + 1 < argc) {
                playerName = argv[++i];
            }
        }
        else if (std::strcmp(argv[i], "-f") == 0 || std::strcmp(argv[i], "--fullscreen") == 0) {
            config.fullscreen = true;
        }
        else if (std::strcmp(argv[i], "-w") == 0 || std::strcmp(argv[i], "--width") == 0) {
            if (i + 1 < argc) {
                try {
                    config.width = static_cast<unsigned int>(std::stoi(argv[++i]));
                } catch (const std::exception&) {
                    std::cerr << "Erreur: Largeur invalide" << std::endl;
                    return 1;
                }
            }
        }
        else if (std::strcmp(argv[i], "--height") == 0) {
            if (i + 1 < argc) {
                try {
                    config.height = static_cast<unsigned int>(std::stoi(argv[++i]));
                } catch (const std::exception&) {
                    std::cerr << "Erreur: Hauteur invalide" << std::endl;
                    return 1;
                }
            }
        }
        else if (std::strcmp(argv[i], "--offline") == 0) {
            offline = true;
        }
    }

    try {
        // Crée le jeu
        rtype::core::RTypeGame game(config);

        // Connecte au serveur si pas en mode offline
        if (!offline) {
            std::cout << "Connexion au serveur " << serverHost << ":" << serverPort << "..." << std::endl;

            if (!game.connectToServer(serverHost, serverPort, playerName)) {
                std::cerr << "Impossible de se connecter au serveur. Démarrage en mode offline." << std::endl;
            } else {
                std::cout << "Connecté!" << std::endl;
            }
        } else {
            std::cout << "Mode hors-ligne activé." << std::endl;
        }

        // Lance le jeu
        game.run();

    } catch (const std::exception& e) {
        std::cerr << "Erreur: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
