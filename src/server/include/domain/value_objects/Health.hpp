/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** Health
*/

#ifndef HEALTH_HPP_
#define HEALTH_HPP_

#include "domain/exceptions/HealthException.hpp"

namespace domain::value_objects {
    class Health {
        private:
            float _healthPoint;
            void validate(float value);

        public:
            explicit Health(float value);
            float value() const;
            Health heal(float value) const;
            Health damage(float value) const;
            bool operator==(const Health& other);
            bool operator<(const Health& other);
    };
}

#endif /* !HEALTH_HPP_ */
