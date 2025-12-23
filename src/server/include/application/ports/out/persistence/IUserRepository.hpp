/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** IUserRepository
*/

#ifndef IUSERREPOSITORY_HPP_
#define IUSERREPOSITORY_HPP_

#include "domain/entities/User.hpp"
#include <optional>
#include <vector>

namespace application::ports::out::persistence {
    using domain::entities::User;
    class IUserRepository {
        public:
            virtual ~IUserRepository() = default;

            virtual void save(const User& user) const = 0;
            virtual void update(const User& user) = 0;
            virtual std::optional<User> findById(const std::string& id) = 0;
            virtual std::optional<User> findByName(const std::string& name) = 0;
            virtual std::optional<User> findByEmail(const std::string& email) = 0;
            virtual std::vector<User> findAll() = 0;

        protected:
        private:
    };
}


#endif /* !IUSERREPOSITORY_HPP_ */
