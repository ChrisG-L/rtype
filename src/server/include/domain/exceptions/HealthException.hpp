/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** HealthException
*/

#ifndef HEALTHEXCEPTION_HPP_
#define HEALTHEXCEPTION_HPP_

#include "DomainException.hpp"

namespace domain::exceptions {
    class HealthException: public DomainException {
        public:
            explicit HealthException(float health): DomainException(buildMessage(health)) {};
        
        private:
            std::string buildMessage(float health) {
                std::ostringstream oss;
                oss << "Health must be over 0 point and minus 5 points: " << health << ".";
                return oss.str();
            }
    };
}
#endif /* !HEALTHEXCEPTION_HPP_ */
