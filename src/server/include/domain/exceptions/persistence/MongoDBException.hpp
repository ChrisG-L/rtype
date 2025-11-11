/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** MongoDBException
*/

#ifndef MONGODBEXCEPTION_HPP_
#define MONGODBEXCEPTION_HPP_

#include "../DomainException.hpp"

namespace domain::exceptions::persistence {
    class MongoDBException: public DomainException {
        public:
            explicit MongoDBException();

        protected:
        private:
            static std::string buildMessage();
    };
}

#endif /* !MONGODBEXCEPTION_HPP_ */
