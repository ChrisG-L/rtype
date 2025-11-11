/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** IUserRespository
*/

#ifndef IUSERRESPOSITORY_HPP_
#define IUSERRESPOSITORY_HPP_

#include "domain/entities/User.hpp"
#include <optional>
#include <vector>

namespace application::ports::out::persistence {
    using domain::entities::User;
    class IUserRespository {
        public:
            virtual ~IUserRespository() = default;

            virtual void save(const User& user) const = 0;
            virtual void update(const User& user) = 0;
            virtual std::optional<User> findById(const std::string& id) const = 0;
            virtual std::optional<User> findByName(const std::string& name) const = 0;
            virtual std::vector<User> findAll() = 0;

        protected:
        private:
    };
}


#endif /* !IUSERRESPOSITORY_HPP_ */
