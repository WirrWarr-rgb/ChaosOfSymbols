#include <sstream>
#include <algorithm>
#include "WorldConfig.h"
#include "Logger.h"

WorldConfig::WorldConfig()
    : m_width(80), m_height(40), m_seed(1337),
    m_useRandomSeed(true), m_noiseFrequency(0.7f),
    m_neighborRadius(3),
    m_worldConfigPath("config/world_gen.cfg"),
    m_spawnConfigPath("config/world_spawn.cfg") {
}

WorldConfig::WorldConfig(const std::string& worldConfigPath, const std::string& spawnConfigPath)
    : m_width(80), m_height(40), m_seed(1337),
    m_useRandomSeed(true), m_noiseFrequency(0.7f),
    m_worldConfigPath(worldConfigPath),
    m_spawnConfigPath(spawnConfigPath) {
}

/// <summary>
/// Загрузка полной конфигурации мира из файлов: основной конфиг + правила спавна
/// </summary>
/// <returns></returns>
bool WorldConfig::LoadConfig() {
    Logger::Log("Loading world generation configuration...");

    bool originalUseRandomSeed = m_useRandomSeed;
    int originalSeed = m_seed;

    if (!LoadFromFile(m_worldConfigPath)) {
        Logger::Log("ERROR: Failed to load world config: " + m_worldConfigPath);
        return false;
    }


    if (m_useRandomSeed) {
        m_seed = static_cast<int>(time(nullptr));
        Logger::Log("Using random seed: " + std::to_string(m_seed));
    }
    else {
        Logger::Log("Using configured seed: " + std::to_string(m_seed));
    }

    if (!ParseSpawnConfig()) {
        Logger::Log("ERROR: Failed to load spawn config: " + m_spawnConfigPath);
        return false;
    }

    Logger::Log("World config loaded successfully: " +
        std::to_string(m_width) + "x" + std::to_string(m_height) +
        ", seed: " + std::to_string(m_seed) +
        ", random: " + std::string(m_useRandomSeed ? "true" : "false"));

    return true;
}

/// <summary>
/// Обработка пары ключ-значение из основного конфига
/// </summary>
bool WorldConfig::ParseKeyValue(const std::string& key, const std::string& value) {
    if (key == "Width" || key == "WorldWidth") {
        m_width = std::stoi(value);
    }
    else if (key == "Height" || key == "WorldHeight") {
        m_height = std::stoi(value);
    }
    else if (key == "Seed" || key == "WorldSeed") {
        m_seed = std::stoi(value);
    }
    else if (key == "UseRandomSeed") {
        m_useRandomSeed = (value == "true");
    }
    else if (key == "NoiseFrequency") {
        m_noiseFrequency = std::stof(value);
    }
    else if (key == "NeighborRadius") {
        m_neighborRadius = std::stoi(value);
    }
    else {
        Logger::Log("WARNING: Unknown config key: " + key);
        return false;
    }

    return true;
}

/// <summary>
/// Загрука и парсинг конфигурации спавна тайлов для разных зон высот
/// </summary>
bool WorldConfig::ParseSpawnConfig() {
    std::ifstream file(m_spawnConfigPath);
    if (!file.is_open()) {
        return false;
    }

    m_spawnRules.clear();

    std::string line;
    while (std::getline(file, line)) {
        size_t commentPos = line.find("//");
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }

        line.erase(0, line.find_first_not_of(" \t"));
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string spawnTileStr, probabilitiesStr;

        if (std::getline(ss, spawnTileStr, '=') &&
            std::getline(ss, probabilitiesStr)) {

            if (spawnTileStr.length() != 1) {
                Logger::Log("WARNING: Invalid spawn tile in config: " + spawnTileStr);
                continue;
            }

            char spawnTile = spawnTileStr[0];

            std::vector<float> zoneProbs;
            std::stringstream probStream(probabilitiesStr);
            std::string probToken;

            while (std::getline(probStream, probToken, ':')) {
                try {
                    zoneProbs.push_back(std::stof(probToken));
                }
                catch (const std::exception& e) {
                    Logger::Log("WARNING: Invalid probability format: " + probToken);
                    zoneProbs.push_back(0.1f);
                }
            }

            while (zoneProbs.size() < 3) {
                zoneProbs.push_back(0.1f);
            }

            SpawnRule rule;
            rule.tileId = -1; 
            rule.character = spawnTile;
            rule.zoneProbabilities = zoneProbs;

            m_spawnRules[spawnTile] = rule;
        }
    }

    file.close();
    return true;
}

/// <summary>
/// Возвращение сида для генерации (случайный или заданный)
/// </summary>
/// <returns></returns>
int WorldConfig::GetEffectiveSeed() const {
    return m_seed;
}

/// <summary>
/// Возвращение правил спавна для указанного символа
/// </summary>
/// <param name="spawnTile"></param>
/// <returns></returns>
const SpawnRule* WorldConfig::GetSpawnRule(char spawnTile) const {
    auto it = m_spawnRules.find(spawnTile);
    if (it != m_spawnRules.end()) {
        return &it->second;
    }
    return nullptr;
}