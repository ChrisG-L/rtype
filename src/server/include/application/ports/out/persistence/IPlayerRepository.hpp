/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** IPlayerRepository
*/

#ifndef IPLAYERREPOSITORY_HPP_
#define IPLAYERREPOSITORY_HPP_

#include "domain/entities/Player.hpp"
#include <optional>


namespace application::ports::out::persistence {
    using domain::entities::Player;
    class IPlayerRepository {
        public:
            virtual ~IPlayerRepository() = default;

            virtual void save(const Player& player) const = 0;
            virtual void update(const Player& player) = 0;
            virtual std::optional<Player> findById(const std::string& id) const = 0;
            virtual std::optional<Player> findByName(const std::string& name) const = 0;
            virtual std::vector<Player> findAll() = 0;

        protected:
        private:
    };
}
#endif /* !IPLAYERREPOSITORY_HPP_ */
