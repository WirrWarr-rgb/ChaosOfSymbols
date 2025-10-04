#include "GameSettings.h"
#include <fstream>
#include <iostream>
#include <ctime>
#include <cstdlib>

GameSettings::GameSettings(const std::string& filePath)
    : m_settingsFilePath(filePath), m_worldSeed(1337), m_useRandomSeed(true) {}

bool GameSettings::LoadFromFile() {
    std::ifstream file(m_settingsFilePath);
    if (!file.is_open()) {
        std::cout << "Settings file not found, using defaults\n";
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.find("WorldSeed=") == 0) {
            m_worldSeed = std::stoi(line.substr(10));
        }
        else if (line.find("UseRandomSeed=") == 0) {
            m_useRandomSeed = (line.substr(14) == "true");
        }
    }

    file.close();
    std::cout << "Settings loaded from: " << m_settingsFilePath << std::endl;
    return true;
}

bool GameSettings::SaveToFile() {
    std::ofstream file(m_settingsFilePath);
    if (!file.is_open()) {
        std::cout << "Failed to save settings to: " << m_settingsFilePath << std::endl;
        return false;
    }

    file << "WorldSeed=" << m_worldSeed << std::endl;
    file << "UseRandomSeed=" << (m_useRandomSeed ? "true" : "false") << std::endl;

    file.close();
    std::cout << "Settings saved to: " << m_settingsFilePath << std::endl;
    return true;
}

int GameSettings::GetWorldSeed() const {
    if (m_useRandomSeed) {
        return static_cast<int>(time(nullptr));
    }
    return m_worldSeed;
}