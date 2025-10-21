#include "WorldConfig.h"
#include "Logger.h"
#include <sstream>
#include <algorithm>

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

bool WorldConfig::LoadConfig() {
    Logger::Log("Loading world generation configuration...");

    // Сохраняем флаг использования случайного сида
    bool shouldUseRandomSeed = m_useRandomSeed;

    if (shouldUseRandomSeed) {
        m_seed = static_cast<int>(time(nullptr));
        Logger::Log("Using random seed: " + std::to_string(m_seed));
    }

    // Загружаем основной конфиг
    if (!LoadFromFile(m_worldConfigPath)) {
        Logger::Log("ERROR: Failed to load world config: " + m_worldConfigPath);
        return false;
    }

    // ВОССТАНАВЛИВАЕМ случайный сид после загрузки конфига, если нужно
    if (shouldUseRandomSeed) {
        // НЕ перезаписываем сид из конфига
        Logger::Log("Keeping random seed: " + std::to_string(m_seed) + " (ignoring config seed)");
    }

    // Загружаем конфиг спавна
    if (!ParseSpawnConfig()) {
        Logger::Log("ERROR: Failed to load spawn config: " + m_spawnConfigPath);
        return false;
    }

    Logger::Log("World config loaded successfully: " +
        std::to_string(m_width) + "x" + std::to_string(m_height) +
        ", seed: " + std::to_string(m_seed) +
        ", random: " + std::string(shouldUseRandomSeed ? "true" : "false"));

    return true;
}

bool WorldConfig::ParseKeyValue(const std::string& key, const std::string& value) {
    if (key == "Width" || key == "WorldWidth") {
        m_width = std::stoi(value);
    }
    else if (key == "Height" || key == "WorldHeight") {
        m_height = std::stoi(value);
    }
    else if (key == "Seed" || key == "WorldSeed") {
        if (!m_useRandomSeed) {
            m_seed = std::stoi(value);
        }
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

bool WorldConfig::ParseSpawnConfig() {
    std::ifstream file(m_spawnConfigPath);
    if (!file.is_open()) {
        return false;
    }

    m_spawnRules.clear();

    std::string line;
    while (std::getline(file, line)) {
        // Удаляем комментарии
        size_t commentPos = line.find("//");
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }

        // Удаляем пробелы
        line.erase(0, line.find_first_not_of(" \t"));
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string spawnTileStr, probabilitiesStr;

        // Парсим формат: символ=вероятности
        if (std::getline(ss, spawnTileStr, '=') &&
            std::getline(ss, probabilitiesStr)) {

            if (spawnTileStr.length() != 1) {
                Logger::Log("WARNING: Invalid spawn tile in config: " + spawnTileStr);
                continue;
            }

            char spawnTile = spawnTileStr[0];

            // Парсим вероятности для трех зон (низины:равнины:горы)
            std::vector<float> zoneProbs;
            std::stringstream probStream(probabilitiesStr);
            std::string probToken;

            while (std::getline(probStream, probToken, ':')) {
                try {
                    zoneProbs.push_back(std::stof(probToken));
                }
                catch (const std::exception& e) {
                    Logger::Log("WARNING: Invalid probability format: " + probToken);
                    zoneProbs.push_back(0.1f); // fallback
                }
            }

            // Дополняем до 3 вероятностей если нужно
            while (zoneProbs.size() < 3) {
                zoneProbs.push_back(0.1f);
            }

            SpawnRule rule;
            rule.tileId = -1; // Заполнится позже через TileManager
            rule.character = spawnTile;
            rule.zoneProbabilities = zoneProbs;

            m_spawnRules[spawnTile] = rule;
        }
    }

    file.close();
    return true;
}

int WorldConfig::GetEffectiveSeed() const {
    return m_seed;
}

const SpawnRule* WorldConfig::GetSpawnRule(char spawnTile) const {
    auto it = m_spawnRules.find(spawnTile);
    if (it != m_spawnRules.end()) {
        return &it->second;
    }
    return nullptr;
}