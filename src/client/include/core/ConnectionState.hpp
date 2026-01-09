/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** ConnectionState - Track TCP/UDP connection status
*/

#ifndef CONNECTIONSTATE_HPP_
#define CONNECTIONSTATE_HPP_

#include <string>

namespace core
{
    enum class ConnectionStatus {
        Disconnected,
        Connecting,
        Connected
    };

    struct ConnectionState {
        ConnectionStatus tcp = ConnectionStatus::Disconnected;
        ConnectionStatus udp = ConnectionStatus::Disconnected;

        bool isFullyConnected() const {
            return tcp == ConnectionStatus::Connected &&
                   udp == ConnectionStatus::Connected;
        }

        bool isFullyDisconnected() const {
            return tcp == ConnectionStatus::Disconnected &&
                   udp == ConnectionStatus::Disconnected;
        }

        bool hasAnyConnection() const {
            return tcp == ConnectionStatus::Connected ||
                   udp == ConnectionStatus::Connected;
        }

        bool needsReconnection() const {
            return tcp != ConnectionStatus::Connected ||
                   udp != ConnectionStatus::Connected;
        }

        std::string getStatusText() const {
            std::string text;
            text += "TCP: ";
            switch (tcp) {
                case ConnectionStatus::Disconnected: text += "Deconnecte"; break;
                case ConnectionStatus::Connecting: text += "Connexion..."; break;
                case ConnectionStatus::Connected: text += "Connecte"; break;
            }
            text += "  |  UDP: ";
            switch (udp) {
                case ConnectionStatus::Disconnected: text += "Deconnecte"; break;
                case ConnectionStatus::Connecting: text += "Connexion..."; break;
                case ConnectionStatus::Connected: text += "Connecte"; break;
            }
            return text;
        }
    };

    enum class ConnectionSceneMode {
        InitialConnection,  // "En attente de connexion..."
        Reconnection        // "Reconnexion en cours..."
    };
}

#endif /* !CONNECTIONSTATE_HPP_ */
