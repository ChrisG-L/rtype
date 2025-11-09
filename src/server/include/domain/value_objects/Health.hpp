/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** Health
*/

#ifndef HEALTH_HPP_
#define HEALTH_HPP_

#include "exceptions/HealthException.hpp"

namespace domain::value_objects {
    class Health {
        private:
            float _healthPoint;

            void validate(float value) {
                if (value < 0.0 || value > 5.0)
                    throw exceptions::HealthException(value);
            }

        public:
            explicit Health(float value): _healthPoint(value) {
                validate(value);
            };

            float value() const {
                _healthPoint;
            }

            Health heal(float value) const {
                Health newHealth(_healthPoint + value);
                return newHealth;
            }

            Health damage(float value) const {
                Health newHealth(_healthPoint - value);
                return newHealth;
            }
    
            bool operator==(const Health& other) {
                return _healthPoint == other._healthPoint;
            }

            bool operator<(const Health& other) {
                return _healthPoint < other._healthPoint;
            }
    };
}

#endif /* !HEALTH_HPP_ */
