/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** UserId
*/

#ifndef USERID_HPP_
#define USERID_HPP_

#include "domain/exceptions/user/UserIdException.hpp"
#include <string>

namespace domain::value_objects::user {
    class UserId {
        private:
            std::string _id;
            void validate(const std::string &id);

        public:
            explicit UserId(const std::string& id);
            std::string value() const;
            bool operator==(const UserId& other) const;
            bool operator!=(const UserId& other) const;
    };
}

#endif /* !USERID_HPP_ */
