/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** Register
*/

#include "application/use_cases/auth/Register.hpp"
#include "domain/exceptions/user/UsernameAlreadyExistsException.hpp"
#include "domain/exceptions/user/EmailAlreadyExistsException.hpp"

namespace application::use_cases::auth {
    Register::Register(
        std::shared_ptr<IUserRepository> userRepository,
        std::shared_ptr<IIdGenerator> idGenerator,
        std::shared_ptr<ILogger> logger
    ) : _userRepository(userRepository), _idGenerator(idGenerator), _logger(logger) {}

    std::optional<User> Register::execute(
        const std::string& username,
        const std::string& email,
        const std::string& unHashedPassword
    ) {
        _logger->info("[AUTH/REGISTER] Attempting registration - username: '{}', email: '{}'",
            username, email);

        auto playerOptByName = _userRepository->findByName(username);
        if (playerOptByName.has_value()) {
            throw domain::exceptions::user::UsernameAlreadyExistsException(username);
        }

        auto playerOptByEmail = _userRepository->findByEmail(email);
        if (playerOptByEmail.has_value()) {
            throw domain::exceptions::user::EmailAlreadyExistsException(email);
        }

        User user(domain::value_objects::user::UserId(_idGenerator->generate()),
            domain::value_objects::user::Username(username),
            domain::value_objects::user::Email((email)),
            domain::value_objects::user::Password(domain::value_objects::user::utils::hashPassword(unHashedPassword)));

        _userRepository->save(user);
        return user;
    }
}