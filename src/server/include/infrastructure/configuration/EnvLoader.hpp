/*
** EPITECH PROJECT, 2025
** rtype
** File description:
** EnvLoader - Loads environment variables from .env file
*/

#ifndef ENVLOADER_HPP_
#define ENVLOADER_HPP_

#include <string>
#include <fstream>
#include <cstdlib>

namespace infrastructure::configuration {

    class EnvLoader {
        public:
            static bool load(const std::string& filepath = ".env") {
                std::ifstream file(filepath);
                if (!file.is_open()) {
                    return false;
                }

                std::string line;
                while (std::getline(file, line)) {
                    if (line.empty() || line[0] == '#') {
                        continue;
                    }

                    size_t pos = line.find('=');
                    if (pos == std::string::npos) {
                        continue;
                    }

                    std::string key = trim(line.substr(0, pos));
                    std::string value = trim(line.substr(pos + 1));

                    value = removeQuotes(value);

                    if (!key.empty()) {
                        setenv(key.c_str(), value.c_str(), 0);
                    }
                }
                return true;
            }

        private:
            static std::string trim(const std::string& str) {
                size_t start = str.find_first_not_of(" \t\r\n");
                if (start == std::string::npos) return "";
                size_t end = str.find_last_not_of(" \t\r\n");
                return str.substr(start, end - start + 1);
            }

            static std::string removeQuotes(const std::string& str) {
                if (str.size() >= 2) {
                    if ((str.front() == '"' && str.back() == '"') ||
                        (str.front() == '\'' && str.back() == '\'')) {
                        return str.substr(1, str.size() - 2);
                    }
                }
                return str;
            }
    };

}

#endif /* !ENVLOADER_HPP_ */
