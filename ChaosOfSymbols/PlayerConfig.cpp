#include "PlayerConfig.h"
#include "Logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>

PlayerConfig::PlayerConfig()
    : m_configPath("config/player.cfg"),
    m_defaultPlayerX(50), m_defaultPlayerY(20),
    m_maxHP(100), m_maxHunger(100),
    m_enableHP(true), m_enableHunger(true),
    m_baseXP(100), m_xpMultiplier(1.5f),
    m_moveCooldownMs(50) {
}

PlayerConfig::PlayerConfig(const std::string& configPath)
    : m_configPath(configPath),
    m_defaultPlayerX(50), m_defaultPlayerY(20),
    m_maxHP(100), m_maxHunger(100),
    m_enableHP(true), m_enableHunger(true),
    m_baseXP(100), m_xpMultiplier(1.5f),
    m_moveCooldownMs(50) {
}

bool PlayerConfig::LoadConfig() {
    Logger::Log("Loading player configuration from: " + m_configPath);

    std::ifstream file(m_configPath);
    if (!file.is_open()) {
        Logger::Log("WARNING: Could not open player config file, using defaults");
        SetDefaults();
        return false;
    }

    std::string line;
    int linesProcessed = 0;

    while (std::getline(file, line)) {
        // Пропускаем пустые строки и комментарии
        if (line.empty() || line[0] == ';' || line[0] == '#') {
            continue;
        }

        // Убираем пробелы в начале и конце
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);

        // Ищем разделитель '='
        size_t delimiterPos = line.find('=');
        if (delimiterPos == std::string::npos) {
            continue;
        }

        std::string key = line.substr(0, delimiterPos);
        std::string value = line.substr(delimiterPos + 1);

        // Убираем пробелы вокруг ключа и значения
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);

        if (ParseKeyValue(key, value)) {
            linesProcessed++;
        }
    }

    file.close();

    Logger::Log("Player configuration loaded: " + std::to_string(linesProcessed) + " parameters");
    Logger::Log("Player config - HP: " + std::to_string(m_maxHP) +
        (m_enableHP ? " (enabled)" : " (disabled)") +
        ", Hunger: " + std::to_string(m_maxHunger) +
        (m_enableHunger ? " (enabled)" : " (disabled)"));

    return true;
}

void PlayerConfig::SetDefaults() {
    Logger::Log("Setting default player configuration");

    m_defaultPlayerX = 50;
    m_defaultPlayerY = 20;
    m_maxHP = 100;
    m_maxHunger = 100;
    m_enableHP = true;
    m_enableHunger = true;
    m_baseXP = 100;
    m_xpMultiplier = 1.5f;
    m_moveCooldownMs = 50;
}

bool PlayerConfig::ParseKeyValue(const std::string& key, const std::string& value) {
    if (key == "DefaultPlayerX") {
        m_defaultPlayerX = std::stoi(value);
    }
    else if (key == "DefaultPlayerY") {
        m_defaultPlayerY = std::stoi(value);
    }
    else if (key == "MAX_HP") {
        m_maxHP = std::stoi(value);
    }
    else if (key == "MAX_HUNGER") {
        m_maxHunger = std::stoi(value);
    }
    else if (key == "EnableHP") {
        m_enableHP = (value == "true" || value == "1");
    }
    else if (key == "EnableHunger") {
        m_enableHunger = (value == "true" || value == "1");
    }
    else if (key == "BaseXP") {
        m_baseXP = std::stoi(value);
    }
    else if (key == "XPMultiplier") {
        m_xpMultiplier = std::stof(value);
    }
    else if (key == "MoveCooldownMs") {
        m_moveCooldownMs = std::stoi(value);
    }
    else {
        Logger::Log("WARNING: Unknown player config key: " + key);
        return false;
    }

    return true;
}