/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** IIdGenerator - Port for generating unique IDs
*/

#ifndef IIDGENERATOR_HPP_
#define IIDGENERATOR_HPP_

#include <string>

namespace application::ports::out {
    class IIdGenerator {
        public:
            virtual ~IIdGenerator() = default;
            virtual std::string generate() = 0;
    };
}

#endif /* !IIDGENERATOR_HPP_ */
