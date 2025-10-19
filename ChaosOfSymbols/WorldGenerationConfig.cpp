#include "WorldGenerationConfig.h"
#include "Logger.h"
#include <fstream>

WorldGenConfig::WorldGenConfig()
    : width(80), height(24), seed(12345), useRandomSeed(true), noiseFrequency(0.05f) {
}

bool WorldGenConfig::LoadFromFile(const std::string& filePath) {
    Logger::Log("Loading world config from: " + filePath);

    std::ifstream file(filePath);
    if (!file.is_open()) {
        Logger::Log("ERROR: World generation config not found: " + filePath);
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        // Удаляем комментарии (всё что после //)
        size_t commentPos = line.find("//");
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }

        // Пропускаем пустые строки
        line.erase(0, line.find_first_not_of(" \t"));
        if (line.empty()) continue;

        size_t delimiterPos = line.find('=');
        if (delimiterPos == std::string::npos) continue;

        std::string key = line.substr(0, delimiterPos);
        std::string value = line.substr(delimiterPos + 1);

        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);

        if (key == "Width") {
            width = std::stoi(value);
        }
        else if (key == "Height") {
            height = std::stoi(value);
        }
        else if (key == "Seed") {
            seed = std::stoi(value);
        }
        else if (key == "UseRandomSeed") {
            useRandomSeed = (value == "true");
        }
        else if (key == "NoiseFrequency") {
            noiseFrequency = std::stof(value);
        }
    }

    file.close();
    return true;
}