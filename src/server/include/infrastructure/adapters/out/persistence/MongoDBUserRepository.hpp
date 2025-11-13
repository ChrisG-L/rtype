/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** MongoDBUserRepository
*/

#ifndef MONGODBUSERREPOSITORY_HPP_
#define MONGODBUSERREPOSITORY_HPP_

#include "../../../configuration/DBConfig.hpp"
#include "domain/entities/User.hpp"

#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>

#include "MongoDBConfiguration.hpp"
#include "application/ports/out/persistence/IUserRespository.hpp"

namespace infrastructure::adapters::out::persistence {
    using application::ports::out::persistence::IUserRespository;
    using domain::entities::User;

    class MongoDBUserRepository: public IUserRespository {
        private:
            static std::shared_ptr<MongoDBConfiguration> _mongoDB;
            std::unique_ptr<mongocxx::v_noabi::collection> _collection;
        public:
            explicit MongoDBUserRepository(std::unique_ptr<MongoDBConfiguration> mongoDB);
            ~MongoDBUserRepository();

            User documentToUser(const bsoncxx::document::view& doc);

            void save(const User& user) const override;
            void update(const User& user) override;
            std::optional<User> findById(const std::string& id) const override;
            std::optional<User> findByName(const std::string& name) const override;
            std::vector<User> findAll() override;
    };
}

#endif /* !MONGODBUSERREPOSITORY_HPP_ */
