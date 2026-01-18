/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** MongoDBUserSettingsRepository - MongoDB implementation for user settings
*/

#ifndef MONGODBUSERSETTINGSREPOSITORY_HPP_
#define MONGODBUSERSETTINGSREPOSITORY_HPP_

#include "application/ports/out/persistence/IUserSettingsRepository.hpp"
#include "MongoDBConfiguration.hpp"

#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/types.hpp>

namespace infrastructure::adapters::out::persistence {

using application::ports::out::persistence::IUserSettingsRepository;
using application::ports::out::persistence::UserSettingsData;

class MongoDBUserSettingsRepository : public IUserSettingsRepository {
private:
    std::shared_ptr<MongoDBConfiguration> _mongoDB;
    static constexpr const char* COLLECTION_NAME = "user_settings";

    UserSettingsData documentToSettings(const bsoncxx::document::view& doc);

public:
    explicit MongoDBUserSettingsRepository(std::shared_ptr<MongoDBConfiguration> mongoDB);
    ~MongoDBUserSettingsRepository() override = default;

    std::optional<UserSettingsData> findByEmail(const std::string& email) override;
    void save(const std::string& email, const UserSettingsData& settings) override;
    void remove(const std::string& email) override;
};

} // namespace infrastructure::adapters::out::persistence

#endif /* !MONGODBUSERSETTINGSREPOSITORY_HPP_ */
