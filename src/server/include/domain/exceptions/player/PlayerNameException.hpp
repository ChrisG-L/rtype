/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** PlayerNameException
*/

#ifndef PLAYERNAMEEXCEPTION_HPP_
#define PLAYERNAMEEXCEPTION_HPP_

#include "../DomainException.hpp"
#include <sstream>

namespace domain::exceptions::player {
    class PlayerNameException: public DomainException {
        public:
            explicit PlayerNameException(std::string name): DomainException(buildMessage(name)) {};

        protected:
        private:
            static std::string buildMessage(std::string name) {
                std::ostringstream oss;
                oss << "The username has to be between 6 and 21 caracters " << name << std::endl;
                return oss.str();
            }
    };
}

#endif /* !PLAYERNAMEEXCEPTION_HPP_ */
