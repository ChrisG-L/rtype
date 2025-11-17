/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** Password
*/

#ifndef PASSWORD_HPP_
#define PASSWORD_HPP_

#include "domain/exceptions/user/PasswordException.hpp"
#include "utils/PasswordUtils.hpp"
#include <string>

namespace domain::value_objects::user {
    class Password {
        private:
            std::string _passwordHash;
            void validate(const std::string &password);

        public:
            explicit Password(const std::string& password);
            std::string value() const;
            bool verify(std::string password);
            bool operator==(const Password& other) const;
            bool operator!=(const Password& other) const;
    };
}

#endif /* !PASSWORD_HPP_ */
