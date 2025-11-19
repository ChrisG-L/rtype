/*
** EPITECH PROJECT, 2025
** R-Type
** File description:
** System - Classe de base pour les systèmes ECS
*/

#pragma once

namespace rtype::ecs {

// Déclaration anticipée
class Registry;

/**
 * @brief Classe de base pour tous les systèmes ECS
 *
 * Les systèmes contiennent la logique du jeu et opèrent sur les entités
 * qui possèdent des combinaisons spécifiques de composants.
 */
class ISystem {
public:
    virtual ~ISystem() = default;

    /**
     * @brief Met à jour le système
     * @param registry Le registre ECS
     * @param dt Delta time en secondes
     */
    virtual void update(Registry& registry, float dt) = 0;

    /**
     * @brief Retourne la priorité du système (plus bas = exécuté en premier)
     */
    [[nodiscard]] virtual int priority() const { return 0; }

    /**
     * @brief Vérifie si le système est activé
     */
    [[nodiscard]] bool isEnabled() const { return m_enabled; }

    /**
     * @brief Active/désactive le système
     */
    void setEnabled(bool enabled) { m_enabled = enabled; }

protected:
    bool m_enabled = true;
};

} // namespace rtype::ecs
