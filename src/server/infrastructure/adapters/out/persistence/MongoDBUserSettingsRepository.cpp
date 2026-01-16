/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** MongoDBUserSettingsRepository - MongoDB implementation for user settings
*/

#include "infrastructure/adapters/out/persistence/MongoDBUserSettingsRepository.hpp"

namespace infrastructure::adapters::out::persistence {

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;
using bsoncxx::builder::basic::make_array;

MongoDBUserSettingsRepository::MongoDBUserSettingsRepository(
    std::shared_ptr<MongoDBConfiguration> mongoDB)
    : _mongoDB(mongoDB)
{
    auto db = _mongoDB->getDatabaseConfig();
    _collection = std::make_unique<mongocxx::v_noabi::collection>(db["user_settings"]);
}

UserSettingsData MongoDBUserSettingsRepository::documentToSettings(
    const bsoncxx::document::view& doc)
{
    UserSettingsData settings;

    // Color blind mode
    if (doc["colorBlindMode"]) {
        settings.colorBlindMode = std::string(doc["colorBlindMode"].get_string().value);
    }

    // Game speed percent
    if (doc["gameSpeedPercent"]) {
        settings.gameSpeedPercent = static_cast<uint16_t>(doc["gameSpeedPercent"].get_int32().value);
    }

    // Key bindings array (with backward compatibility for old 12-element format)
    // First set defaults for all 26 bindings (in case DB has old format)
    settings.setDefaultKeyBindings();

    if (doc["keyBindings"] && doc["keyBindings"].type() == bsoncxx::type::k_array) {
        auto arr = doc["keyBindings"].get_array().value;
        size_t i = 0;
        for (auto&& elem : arr) {
            if (i >= settings.keyBindings.size()) break;
            settings.keyBindings[i] = static_cast<uint8_t>(elem.get_int32().value);
            ++i;
        }
        // If old format (12 elements), indices 12-25 keep their defaults set above
    }

    // Ship skin (default to 1 if not present)
    if (doc["shipSkin"]) {
        settings.shipSkin = static_cast<uint8_t>(doc["shipSkin"].get_int32().value);
    } else {
        settings.shipSkin = 1;
    }

    // Voice settings (with defaults)
    settings.voiceMode = doc["voiceMode"]
        ? static_cast<uint8_t>(doc["voiceMode"].get_int32().value) : 0;
    settings.vadThreshold = doc["vadThreshold"]
        ? static_cast<uint8_t>(doc["vadThreshold"].get_int32().value) : 2;
    settings.micGain = doc["micGain"]
        ? static_cast<uint8_t>(doc["micGain"].get_int32().value) : 100;
    settings.voiceVolume = doc["voiceVolume"]
        ? static_cast<uint8_t>(doc["voiceVolume"].get_int32().value) : 100;

    // Audio device selection (empty string = auto)
    if (doc["audioInputDevice"]) {
        settings.audioInputDevice = std::string(doc["audioInputDevice"].get_string().value);
    }
    if (doc["audioOutputDevice"]) {
        settings.audioOutputDevice = std::string(doc["audioOutputDevice"].get_string().value);
    }

    // Chat settings
    settings.keepChatOpenAfterSend = doc["keepChatOpenAfterSend"]
        ? doc["keepChatOpenAfterSend"].get_bool().value : false;

    // Hidden GodMode (default false)
    settings.godMode = doc["godMode"]
        ? doc["godMode"].get_bool().value : false;

    return settings;
}

std::optional<UserSettingsData> MongoDBUserSettingsRepository::findByEmail(
    const std::string& email)
{
    auto result = _collection->find_one(make_document(kvp("email", email)));
    if (result.has_value()) {
        return documentToSettings(result->view());
    }
    return std::nullopt;
}

void MongoDBUserSettingsRepository::save(
    const std::string& email,
    const UserSettingsData& settings)
{
    // Build key bindings array
    bsoncxx::builder::basic::array keyBindingsArray;
    for (const auto& key : settings.keyBindings) {
        keyBindingsArray.append(static_cast<int32_t>(key));
    }

    auto doc = make_document(
        kvp("email", email),
        kvp("colorBlindMode", settings.colorBlindMode),
        kvp("gameSpeedPercent", static_cast<int32_t>(settings.gameSpeedPercent)),
        kvp("keyBindings", keyBindingsArray),
        kvp("shipSkin", static_cast<int32_t>(settings.shipSkin)),
        kvp("voiceMode", static_cast<int32_t>(settings.voiceMode)),
        kvp("vadThreshold", static_cast<int32_t>(settings.vadThreshold)),
        kvp("micGain", static_cast<int32_t>(settings.micGain)),
        kvp("voiceVolume", static_cast<int32_t>(settings.voiceVolume)),
        kvp("audioInputDevice", settings.audioInputDevice),
        kvp("audioOutputDevice", settings.audioOutputDevice),
        kvp("keepChatOpenAfterSend", settings.keepChatOpenAfterSend),
        kvp("godMode", settings.godMode),
        kvp("updatedAt", bsoncxx::types::b_date{std::chrono::system_clock::now()})
    );

    // Upsert: update if exists, insert if not
    auto filter = make_document(kvp("email", email));
    mongocxx::options::update options;
    options.upsert(true);

    _collection->update_one(
        filter.view(),
        make_document(kvp("$set", doc)),
        options
    );
}

void MongoDBUserSettingsRepository::remove(const std::string& email)
{
    _collection->delete_one(make_document(kvp("email", email)));
}

} // namespace infrastructure::adapters::out::persistence
