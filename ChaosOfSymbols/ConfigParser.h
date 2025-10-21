#pragma once
#include <string>
#include <fstream>

class ConfigParser {
protected:
    virtual bool ParseKeyValue(const std::string& key, const std::string& value) = 0;

public:
    virtual ~ConfigParser() = default;

    bool LoadFromFile(const std::string& filePath) {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            return false;
        }

        std::string line;
        while (std::getline(file, line)) {
            // Удаляем комментарии
            size_t commentPos = line.find("//");
            if (commentPos != std::string::npos) {
                line = line.substr(0, commentPos);
            }

            // Удаляем начальные и конечные пробелы
            line.erase(0, line.find_first_not_of(" \t"));
            line.erase(line.find_last_not_of(" \t") + 1);

            if (line.empty()) continue;

            // Парсим ключ=значение
            size_t delimiterPos = line.find('=');
            if (delimiterPos == std::string::npos) continue;

            std::string key = line.substr(0, delimiterPos);
            std::string value = line.substr(delimiterPos + 1);

            // Очищаем ключ и значение от пробелов
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);

            if (!ParseKeyValue(key, value)) {
                file.close();
                return false;
            }
        }

        file.close();
        return true;
    }
};