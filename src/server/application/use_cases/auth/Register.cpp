/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** Register
*/

#include "application/use_cases/auth/Register.hpp"

namespace application::use_cases::auth {
    Register::Register(std::shared_ptr<IUserRepository> userRepository) : _userRepository(userRepository) {}

    std::optional<User> Register::execute(
        const std::string& username,
        const std::string& email,
        const std::string& unHashedPassword
    ) {
        auto playerOptByName = _userRepository->findByName(username);
        if (playerOptByName.has_value()) {
            return std::nullopt;
        }

        auto playerOptByEmail = _userRepository->findByEmail(email);
        if (playerOptByEmail.has_value()) {
            return std::nullopt;
        }

        User user(domain::value_objects::user::UserId(bsoncxx::oid().to_string()),
            domain::value_objects::user::Username(username),
            domain::value_objects::user::Email((email)),
            domain::value_objects::user::Password(domain::value_objects::user::utils::hashPassword(unHashedPassword)));
        _userRepository->save(user);
        return user;
    }
}