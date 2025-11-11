/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** MongoDBPlayerRepository
*/

#ifndef MONGODBPLAYERREPOSITORY_HPP_
#define MONGODBPLAYERREPOSITORY_HPP_

#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>

namespace infrastructure::adapters::out::persistence {
    class MongoDBPlayerRepository {
        public:
            MongoDBPlayerRepository();
            ~MongoDBPlayerRepository();

        protected:
        private:
    };
}

#endif /* !MONGODBPLAYERREPOSITORY_HPP_ */
