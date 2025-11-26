// /*
// ** EPITECH PROJECT, 2025
// ** rtype
// ** File description:
// ** SFMLPlugin
// */


// #include "plugins/SFMLPlugin.hpp"

// namespace sfml {
//     SFMLPlugin::SFMLPlugin() {};

//     const char* SFMLPlugin::getName() const {
//         return "SFML Graphic";
//     }

//     std::unique_ptr<graphics::IWindow> createWindow(
//         Vec2u winSize,
//         const std::string& name
//     ) {
//         return std::make_unique<SFMLWindow>(winSize, name);
//     }
// }


// extern "C" {
//     graphics::IGraphicPlugin* createGraphPlugin() {
//         return new sfml::SFMLPlugin();
//     };

//     // Add destroy plugin
//     const char* getPluginName() {
//         return "SFML Plugin";
//     };
//     const char* getPluginVersion() {
//         return "0.0.1";
//     };
// }
