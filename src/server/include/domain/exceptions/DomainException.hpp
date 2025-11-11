/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** DomainException
*/

#ifndef DOMAINEXCEPTION_HPP_
#define DOMAINEXCEPTION_HPP_

#include <exception>
#include <string>
#include <sstream>

namespace domain::exceptions {
    class DomainException: public std::exception {
        private:
            std::string _message;
        public:
            explicit DomainException(const std::string& message);
            const char* what() const noexcept override;
    };
}
#endif /* !DOMAINEXCEPTION_HPP_ */
