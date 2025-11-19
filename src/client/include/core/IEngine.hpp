/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu-24.04]
** File description:
** IEngine
*/

#ifndef IENGINE_HPP_
#define IENGINE_HPP_

namespace core {
    class IEngine {
        public:
            virtual ~IEngine() = default;

            virtual void initialize() = 0;
            virtual void run() = 0;
    };
}

#endif /* !IENGINE_HPP_ */
