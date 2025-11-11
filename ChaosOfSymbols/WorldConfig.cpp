#include <sstream>
#include <algorithm>
#include <ctime>
#include "WorldConfig.h"
#include "Logger.h"

WorldConfig::WorldConfig()
    : m_width(80), m_height(40), m_seed(1337),
    m_noiseFrequency(0.7f), m_neighborRadius(3),
    m_generationMode(WorldGenerationMode::RANDOM),
    m_worldConfigPath("config/world_gen.cfg"),
    m_spawnConfigPath("config/world_spawn.cfg") {
}

/// <summary>
/// Загрузка полной конфигурации мира из файлов: основной конфиг + правила спавна
/// </summary>
/// <returns></returns>
bool WorldConfig::LoadConfig(bool forceReload) {
    if (!forceReload && m_parametersLoadedFromSave) {
        Logger::Log("Using parameters from save, skipping file config load");
        return true;
    }

    Logger::Log("Loading world generation configuration...");

    if (!LoadFromFile(m_worldConfigPath)) {
        Logger::Log("ERROR: Failed to load world config: " + m_worldConfigPath);
        return false;
    }

    // Для режима загрузки карты из файла не нужны дополнительные настройки
    if (m_generationMode == WorldGenerationMode::FROM_MAP_FILE) {
        Logger::Log("Using map from file: " + m_mapFilePath);
        return true;
    }

    // Для режима RANDOM генерируем случайный сид
    if (m_generationMode == WorldGenerationMode::RANDOM) {
        m_seed = static_cast<int>(time(nullptr));
        Logger::Log("Using random seed: " + std::to_string(m_seed));
    }
    else if (m_generationMode == WorldGenerationMode::SEEDED) {
        Logger::Log("Using configured seed: " + std::to_string(m_seed));
    }

    if (!ParseSpawnConfig()) {
        Logger::Log("ERROR: Failed to load spawn config: " + m_spawnConfigPath);
        return false;
    }

    Logger::Log("World config loaded successfully: " +
        std::to_string(m_width) + "x" + std::to_string(m_height) +
        ", seed: " + std::to_string(m_seed) +
        ", mode: " + std::to_string(static_cast<int>(m_generationMode)));

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
    else if (key == "NoiseFrequency") {
        m_noiseFrequency = std::stof(value);
    }
    else if (key == "NeighborRadius") {
        m_neighborRadius = std::stoi(value);
    }
    else if (key == "GenerationMode") {
        int mode = std::stoi(value);
        if (mode >= 0 && mode <= 2) {
            m_generationMode = static_cast<WorldGenerationMode>(mode);
        }
        else {
            Logger::Log("WARNING: Invalid GenerationMode value: " + value + ", using RANDOM");
            m_generationMode = WorldGenerationMode::RANDOM;
        }
    }
    else if (key == "MapFilePath") {
        m_mapFilePath = value;
    }
    else if (key == "WorldName") {
    }
    else if (key == "UseRandomSeed") {
        Logger::Log("WARNING: UseRandomSeed is deprecated, use GenerationMode instead");
        bool useRandom = (value == "true");
        if (useRandom) {
            m_generationMode = WorldGenerationMode::RANDOM;
        }
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
/// Возвращение сида для генерации
/// </summary>
int WorldConfig::GetEffectiveSeed() const {
    return m_seed;
}

/// <summary>
/// Возвращение правил спавна для указанного символа
/// </summary>
const SpawnRule* WorldConfig::GetSpawnRule(char spawnTile) const {
    auto it = m_spawnRules.find(spawnTile);
    if (it != m_spawnRules.end()) {
        return &it->second;
    }
    return nullptr;
}