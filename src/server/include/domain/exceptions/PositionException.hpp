/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** PositionException
*/

#ifndef POSITIONEXCEPTION_HPP_
#define POSITIONEXCEPTION_HPP_

#include "DomainException.hpp"

namespace domain::exceptions {
    class PositionException: public DomainException {
        public:
            explicit PositionException(float x, float y, float z);

        private:
            static std::string buildMessage(float x, float y, float z);
    };
}

#endif /* !POSITIONEXCEPTION_HPP_ */
