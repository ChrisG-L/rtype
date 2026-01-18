/*
** EPITECH PROJECT, 2025
** rtype [WSL: Ubuntu]
** File description:
** MongoDBUserRepository
*/

#ifndef MONGODBUSERREPOSITORY_HPP_
#define MONGODBUSERREPOSITORY_HPP_

#include "domain/entities/User.hpp"

#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/types.hpp>

#include "MongoDBConfiguration.hpp"
#include "application/ports/out/persistence/IUserRepository.hpp"

namespace infrastructure::adapters::out::persistence {
    using application::ports::out::persistence::IUserRepository;
    using domain::entities::User;

    class MongoDBUserRepository: public IUserRepository {
        private:
            std::shared_ptr<MongoDBConfiguration> _mongoDB;
            static constexpr const char* COLLECTION_NAME = "user";

            // Helper to get collection from a pooled client (thread-safe)
            mongocxx::collection getCollection();

        public:
            explicit MongoDBUserRepository(std::shared_ptr<MongoDBConfiguration> mongoDB);
            ~MongoDBUserRepository();

            User documentToUser(const bsoncxx::document::view& doc);

            bsoncxx::types::b_date timePointToDate(const std::chrono::system_clock::time_point& tp) const;
            void save(const User& user) const override;
            void update(const User& user) override;
            std::optional<User> findById(const std::string& id) override;
            std::optional<User> findByName(const std::string& name) override;
            std::optional<User> findByEmail(const std::string& email) override;
            std::vector<User> findAll() override;
    };
}

#endif /* !MONGODBUSERREPOSITORY_HPP_ */
