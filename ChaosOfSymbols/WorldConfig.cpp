#include <sstream>
#include <algorithm>
#include <ctime>
#include <filesystem>
#include "WorldConfig.h"
#include "Logger.h"

namespace fs = std::filesystem;

WorldConfig::WorldConfig()
    : m_width(80), m_height(40), m_seed(1337),
    m_noiseFrequency(0.7f), m_neighborRadius(3),
    m_generationMode(WorldGenerationMode::RANDOM),
    m_worldConfigPath("config/world_gen.cfg"),
    m_spawnConfigPath("config/world_spawn.cfg"),
    m_worldName("New_World"),
    m_playerStartX(40), m_playerStartY(20),
    m_playerMaxHP(30), m_playerMaxHunger(20),
    m_enableHP(true), m_enableHunger(true),
    m_enableEnemies(false), m_enemySpawnRate(0.1f) {
}

WorldConfig::WorldConfig(const std::string& worldConfigPath, const std::string& spawnConfigPath)
    : m_width(80), m_height(40), m_seed(1337),
    m_noiseFrequency(0.7f), m_neighborRadius(3),
    m_generationMode(WorldGenerationMode::RANDOM),
    m_worldConfigPath(worldConfigPath),
    m_spawnConfigPath(spawnConfigPath),
    m_worldName("New_World"),
    m_playerStartX(40), m_playerStartY(20),
    m_playerMaxHP(30), m_playerMaxHunger(20),
    m_enableHP(true), m_enableHunger(true),
    m_enableEnemies(false), m_enemySpawnRate(0.1f) {
}

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

    if (m_generationMode == WorldGenerationMode::FROM_MAP_FILE) {
        Logger::Log("Using map from file: " + m_mapFilePath);
        return true;
    }

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
        m_worldName = value;
    }
    else if (key == "PlayerStartX") {
        m_playerStartX = std::stoi(value);
    }
    else if (key == "PlayerStartY") {
        m_playerStartY = std::stoi(value);
    }
    else if (key == "PlayerMaxHP") {
        m_playerMaxHP = std::stoi(value);
    }
    else if (key == "PlayerMaxHunger") {
        m_playerMaxHunger = std::stoi(value);
    }
    else if (key == "EnableHP") {
        m_enableHP = (value == "true" || value == "1");
    }
    else if (key == "EnableHunger") {
        m_enableHunger = (value == "true" || value == "1");
    }
    else if (key == "EnableEnemies") {
        m_enableEnemies = (value == "true" || value == "1");
    }
    else if (key == "EnemySpawnRate") {
        m_enemySpawnRate = std::stof(value);
    }
    else if (key == "SurvivalRules") {
        m_survivalRules = value;
    }
    else if (key == "BirthRules") {
        m_birthRules = value;
    }
    else if (key == "DeathRules") {
        m_deathRules = value;
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

            // Также сохраняем в tileProbabilities для обратной совместимости
            m_tileProbabilities[spawnTile] = zoneProbs;
        }
    }

    file.close();
    return true;
}

bool WorldConfig::SaveToDirectory(const std::string& directory) const {
    Logger::Log("=== SAVING WORLD CONFIG TO: " + directory + " ===");

    if (!fs::exists(directory)) {
        if (!fs::create_directories(directory)) {
            Logger::Log("ERROR: Failed to create directory: " + directory);
            return false;
        }
        Logger::Log("Created directory: " + directory);
    }

    // Сохраняем основной конфиг
    std::string worldConfigPath = directory + "/world_gen.cfg";
    std::ofstream worldFile(worldConfigPath);

    if (!worldFile.is_open()) {
        Logger::Log("ERROR: Cannot open world config file: " + worldConfigPath);
        return false;
    }

    worldFile << "WorldName=" << m_worldName << "\n";
    worldFile << "Width=" << m_width << "\n";
    worldFile << "Height=" << m_height << "\n";
    worldFile << "Seed=" << m_seed << "\n";
    worldFile << "NoiseFrequency=" << m_noiseFrequency << "\n";
    worldFile << "NeighborRadius=" << m_neighborRadius << "\n";
    worldFile << "GenerationMode=" << static_cast<int>(m_generationMode) << "\n";
    worldFile << "PlayerStartX=" << m_playerStartX << "\n";
    worldFile << "PlayerStartY=" << m_playerStartY << "\n";
    worldFile << "PlayerMaxHP=" << m_playerMaxHP << "\n";
    worldFile << "PlayerMaxHunger=" << m_playerMaxHunger << "\n";
    worldFile << "EnableHP=" << (m_enableHP ? "true" : "false") << "\n";
    worldFile << "EnableHunger=" << (m_enableHunger ? "true" : "false") << "\n";
    worldFile << "EnableEnemies=" << (m_enableEnemies ? "true" : "false") << "\n";
    worldFile << "EnemySpawnRate=" << m_enemySpawnRate << "\n";

    if (!m_survivalRules.empty()) {
        worldFile << "SurvivalRules=" << m_survivalRules << "\n";
    }
    if (!m_birthRules.empty()) {
        worldFile << "BirthRules=" << m_birthRules << "\n";
    }
    if (!m_deathRules.empty()) {
        worldFile << "DeathRules=" << m_deathRules << "\n";
    }

    worldFile.close();
    Logger::Log("Saved world config to: " + worldConfigPath);

    // Сохраняем правила спавна
    std::string spawnConfigPath = directory + "/world_spawn.cfg";
    std::ofstream spawnFile(spawnConfigPath);

    if (!spawnFile.is_open()) {
        Logger::Log("ERROR: Cannot open spawn config file: " + spawnConfigPath);
        return false;
    }

    for (const auto& pair : m_tileProbabilities) {
        const std::vector<float>& probs = pair.second;
        if (probs.size() >= 3) {
            spawnFile << pair.first << "="
                << probs[0] << ":"
                << probs[1] << ":"
                << probs[2] << "\n";
        }
    }

    spawnFile.close();
    Logger::Log("Saved spawn config to: " + spawnConfigPath);

    return true;
}

bool WorldConfig::LoadFromDirectory(const std::string& directory) {
    Logger::Log("=== LOADING WORLD CONFIG FROM: " + directory + " ===");

    std::string worldConfigPath = directory + "/world_gen.cfg";
    std::string spawnConfigPath = directory + "/world_spawn.cfg";

    if (!fs::exists(worldConfigPath) || !fs::exists(spawnConfigPath)) {
        Logger::Log("ERROR: Config files not found in directory: " + directory);
        return false;
    }

    m_worldConfigPath = worldConfigPath;
    m_spawnConfigPath = spawnConfigPath;

    return LoadConfig(true);
}

void WorldConfig::FromEditorConfig(const WorldConfig& editorConfig) {
    // Копируем все параметры из editorConfig
    m_width = editorConfig.m_width;
    m_height = editorConfig.m_height;
    m_seed = editorConfig.m_seed;
    m_noiseFrequency = editorConfig.m_noiseFrequency;
    m_neighborRadius = editorConfig.m_neighborRadius;
    m_generationMode = editorConfig.m_generationMode;
    m_worldName = editorConfig.m_worldName;
    m_playerStartX = editorConfig.m_playerStartX;
    m_playerStartY = editorConfig.m_playerStartY;
    m_playerMaxHP = editorConfig.m_playerMaxHP;
    m_playerMaxHunger = editorConfig.m_playerMaxHunger;
    m_enableHP = editorConfig.m_enableHP;
    m_enableHunger = editorConfig.m_enableHunger;
    m_tileProbabilities = editorConfig.m_tileProbabilities;
    m_survivalRules = editorConfig.m_survivalRules;
    m_birthRules = editorConfig.m_birthRules;
    m_deathRules = editorConfig.m_deathRules;
    m_enableEnemies = editorConfig.m_enableEnemies;
    m_enemySpawnRate = editorConfig.m_enemySpawnRate;

    // Обновляем spawnRules из tileProbabilities
    m_spawnRules.clear();
    for (const auto& pair : m_tileProbabilities) {
        SpawnRule rule;
        rule.character = pair.first;
        rule.zoneProbabilities = pair.second;
        m_spawnRules[pair.first] = rule;
    }
}

WorldConfig WorldConfig::ToEditorConfig() const {
    WorldConfig editorConfig;
    editorConfig.m_width = m_width;
    editorConfig.m_height = m_height;
    editorConfig.m_seed = m_seed;
    editorConfig.m_noiseFrequency = m_noiseFrequency;
    editorConfig.m_neighborRadius = m_neighborRadius;
    editorConfig.m_generationMode = m_generationMode;
    editorConfig.m_worldName = m_worldName;
    editorConfig.m_playerStartX = m_playerStartX;
    editorConfig.m_playerStartY = m_playerStartY;
    editorConfig.m_playerMaxHP = m_playerMaxHP;
    editorConfig.m_playerMaxHunger = m_playerMaxHunger;
    editorConfig.m_enableHP = m_enableHP;
    editorConfig.m_enableHunger = m_enableHunger;
    editorConfig.m_tileProbabilities = m_tileProbabilities;
    editorConfig.m_survivalRules = m_survivalRules;
    editorConfig.m_birthRules = m_birthRules;
    editorConfig.m_deathRules = m_deathRules;
    editorConfig.m_enableEnemies = m_enableEnemies;
    editorConfig.m_enemySpawnRate = m_enemySpawnRate;

    // Копируем spawnRules в tileProbabilities
    for (const auto& pair : m_spawnRules) {
        editorConfig.m_tileProbabilities[pair.first] = pair.second.zoneProbabilities;
    }

    return editorConfig;
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