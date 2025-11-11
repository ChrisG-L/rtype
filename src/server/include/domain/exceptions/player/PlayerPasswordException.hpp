/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** PlayerPasswordException
*/

#ifndef PLAYERPASSWORDEXCEPTION_HPP_
#define PLAYERPASSWORDEXCEPTION_HPP_

#include "../DomainException.hpp"
#include <sstream>

namespace domain::exceptions::player {
    class PlayerPasswordException: public DomainException {
        public:
            explicit PlayerPasswordException(std::string password): DomainException(buildMessage(password)) {};

        protected:
        private:
            static std::string buildMessage(std::string password) {
                std::ostringstream oss;
                oss << "The password has to be more than 6 caracters " << password << ".";
                return oss.str();
            }
    };
}

#endif /* !PLAYERPASSWORDEXCEPTION_HPP_ */
