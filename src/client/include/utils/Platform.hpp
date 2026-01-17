/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** Platform - Cross-platform utility functions
*/

#ifndef PLATFORM_HPP_
#define PLATFORM_HPP_

#include <string>
#include <cstdlib>

namespace client::utils {

/**
 * @brief Opens a URL in the default system browser
 * @param url The URL to open (must be a valid http:// or https:// URL)
 * @note This function is safe to call with hardcoded URLs only.
 *       Never pass user input directly to this function.
 */
inline void openUrlInBrowser(const std::string& url) {
    // Security: Only allow http/https URLs
    if (url.find("http://") != 0 && url.find("https://") != 0) {
        return;
    }

    #ifdef _WIN32
    std::string cmd = "start \"\" \"" + url + "\"";
    std::system(cmd.c_str());
    #elif __APPLE__
    std::string cmd = "open \"" + url + "\"";
    std::system(cmd.c_str());
    #else
    std::string cmd = "xdg-open \"" + url + "\" &";
    std::system(cmd.c_str());
    #endif
}

} // namespace client::utils

#endif /* !PLATFORM_HPP_ */
