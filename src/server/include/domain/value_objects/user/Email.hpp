/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** Email
*/

#ifndef EMAIL_HPP_
#define EMAIL_HPP_

#include "domain/exceptions/user/EmailException.hpp"
#include <string>

namespace domain::value_objects::user {
    class Email {
        private:
            std::string _email;
            void validate(const std::string &email);

        public:
            explicit Email(const std::string& email);
            std::string value() const;
            bool operator==(const Email& other) const;
            bool operator!=(const Email& other) const;
    };
}

#endif /* !EMAIL_HPP_ */
