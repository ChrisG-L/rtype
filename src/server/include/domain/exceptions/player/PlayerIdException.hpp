/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** PlayerIdException
*/

#ifndef PLAYERIDEXCEPTION_HPP_
#define PLAYERIDEXCEPTION_HPP_

#include "../DomainException.hpp"

namespace domain::exceptions::player {
    class PlayerIdException: public DomainException {
        public:
            explicit PlayerIdException(std::string id);

        private:
            static std::string buildMessage(std::string id);
    };
}
#endif /* !PLAYERIDEXCEPTION_HPP_ */
