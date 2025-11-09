/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** PlayerIdException
*/

#ifndef PLAYERIDEXCEPTION_HPP_
#define PLAYERIDEXCEPTION_HPP_

#include "DomainException.hpp"

namespace domain::exceptions::player {
    class PlayerIdException: public DomainException {
        public:
            explicit PlayerIdException(std::string id);

        protected:
        private:
            std::string buildMessage(std::string id) {
                std::ostringstream oss;
                oss << "Player id must be conform to UUID v7 standards: " << id << ".";
                return oss.str();
            }
    };
}
#endif /* !PLAYERIDEXCEPTION_HPP_ */
