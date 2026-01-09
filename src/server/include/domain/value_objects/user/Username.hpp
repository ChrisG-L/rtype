/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** Username
*/

#ifndef USERNAME_HPP_
#define USERNAME_HPP_

#include "domain/exceptions/user/UsernameException.hpp"
#include <string>

namespace domain::value_objects::user {
    class Username {
        private:
            std::string _username;
            void validate(const std::string &username);

        public:
            explicit Username(const std::string& username);
            std::string value() const;
            bool operator==(const Username& other) const;
            bool operator!=(const Username& other) const;
    };
}

#endif /* !USERNAME_HPP_ */
