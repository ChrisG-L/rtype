/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** Health
*/

#include "domain/value_objects/Health.hpp"

namespace domain::value_objects {
    void Health::validate(float value) {
        if (value < 0.0 || value > 5.0)
            throw exceptions::HealthException(value);
    }

    Health::Health(float value) : _healthPoint(value) {
        validate(value);
    }

    float Health::value() const {
        return _healthPoint;
    }

    Health Health::heal(float value) const {
        Health newHealth(_healthPoint + value);
        return newHealth;
    }

    Health Health::damage(float value) const {
        Health newHealth(_healthPoint - value);
        return newHealth;
    }

    bool Health::operator==(const Health& other) {
        return _healthPoint == other._healthPoint;
    }

    bool Health::operator<(const Health& other) {
        return _healthPoint < other._healthPoint;
    }
}
