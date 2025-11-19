/*
** EPITECH PROJECT, 2025
** R-Type
** File description:
** AssetManager - Gestionnaire de ressources (textures, sons, fonts)
*/

#pragma once

#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Audio/SoundBuffer.hpp>
#include <unordered_map>
#include <string>
#include <memory>
#include <filesystem>

namespace rtype::graphics {

/**
 * @brief Gestionnaire centralisé des ressources du jeu
 *
 * Charge et cache les textures, fonts et sons pour éviter
 * les chargements multiples et faciliter la gestion mémoire.
 */
class AssetManager {
public:
    AssetManager() = default;
    ~AssetManager() = default;

    // Non-copiable (singleton potentiel)
    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;

    /**
     * @brief Définit le chemin de base pour les assets
     */
    void setBasePath(const std::string& path) {
        m_basePath = path;
    }

    // ==================== Textures ====================

    /**
     * @brief Charge une texture depuis un fichier
     * @param id Identifiant unique de la texture
     * @param filename Chemin du fichier (relatif au basePath)
     * @return true si le chargement réussit
     */
    bool loadTexture(const std::string& id, const std::string& filename) {
        auto texture = std::make_unique<sf::Texture>();

        std::filesystem::path path = m_basePath / filename;

        if (!texture->loadFromFile(path.string())) {
            return false;
        }

        m_textures[id] = std::move(texture);
        return true;
    }

    /**
     * @brief Récupère une texture par son ID
     * @return Pointeur vers la texture ou nullptr si non trouvée
     */
    [[nodiscard]] sf::Texture* getTexture(const std::string& id) {
        auto it = m_textures.find(id);
        if (it == m_textures.end()) return nullptr;
        return it->second.get();
    }

    /**
     * @brief Vérifie si une texture existe
     */
    [[nodiscard]] bool hasTexture(const std::string& id) const {
        return m_textures.find(id) != m_textures.end();
    }

    // ==================== Fonts ====================

    /**
     * @brief Charge une police depuis un fichier
     */
    bool loadFont(const std::string& id, const std::string& filename) {
        auto font = std::make_unique<sf::Font>();

        std::filesystem::path path = m_basePath / filename;

        if (!font->openFromFile(path.string())) {
            return false;
        }

        m_fonts[id] = std::move(font);
        return true;
    }

    /**
     * @brief Récupère une police par son ID
     */
    [[nodiscard]] sf::Font* getFont(const std::string& id) {
        auto it = m_fonts.find(id);
        if (it == m_fonts.end()) return nullptr;
        return it->second.get();
    }

    // ==================== Sons ====================

    /**
     * @brief Charge un son depuis un fichier
     */
    bool loadSound(const std::string& id, const std::string& filename) {
        auto buffer = std::make_unique<sf::SoundBuffer>();

        std::filesystem::path path = m_basePath / filename;

        if (!buffer->loadFromFile(path.string())) {
            return false;
        }

        m_sounds[id] = std::move(buffer);
        return true;
    }

    /**
     * @brief Récupère un buffer de son par son ID
     */
    [[nodiscard]] sf::SoundBuffer* getSound(const std::string& id) {
        auto it = m_sounds.find(id);
        if (it == m_sounds.end()) return nullptr;
        return it->second.get();
    }

    // ==================== Utilitaires ====================

    /**
     * @brief Libère toutes les ressources
     */
    void clear() {
        m_textures.clear();
        m_fonts.clear();
        m_sounds.clear();
    }

    /**
     * @brief Libère une texture spécifique
     */
    void unloadTexture(const std::string& id) {
        m_textures.erase(id);
    }

    /**
     * @brief Retourne le nombre de textures chargées
     */
    [[nodiscard]] std::size_t getTextureCount() const {
        return m_textures.size();
    }

private:
    std::filesystem::path m_basePath = "assets";

    std::unordered_map<std::string, std::unique_ptr<sf::Texture>> m_textures;
    std::unordered_map<std::string, std::unique_ptr<sf::Font>> m_fonts;
    std::unordered_map<std::string, std::unique_ptr<sf::SoundBuffer>> m_sounds;
};

} // namespace rtype::graphics
