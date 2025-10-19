#include "WorldSpawnConfig.h"
#include "Logger.h"
#include <fstream>
#include <sstream>

WorldSpawnConfig::WorldSpawnConfig(const std::string& configPath)
    : m_configPath(configPath) {
}

bool WorldSpawnConfig::LoadFromFile() {
    Logger::Log("Loading spawn config from: " + m_configPath);

    std::ifstream file(m_configPath);
    if (!file.is_open()) {
        Logger::Log("ERROR: Spawn config not found: " + m_configPath);
        return false;
    }

    m_spawnRules.clear();

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

        std::stringstream ss(line);
        std::string spawnTileStr, probabilitiesStr;

        if (std::getline(ss, spawnTileStr, '=') &&
            std::getline(ss, probabilitiesStr)) {

            if (spawnTileStr.length() != 1) {
                Logger::Log("WARNING: Invalid spawn tile in config: " + spawnTileStr);
                continue;
            }

            char spawnTile = spawnTileStr[0];

            // Парсим вероятности для трех зон
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

            // Если указано меньше 3 вероятностей, дополняем
            while (zoneProbs.size() < 3) {
                zoneProbs.push_back(0.1f);
            }

            SpawnRule rule;
            rule.tileId = -1;
            rule.zoneProbabilities = zoneProbs;
            rule.character = spawnTile;

            m_spawnRules[spawnTile] = rule;
            Logger::Log("Added spawn rule: '" + std::string(1, spawnTile) +
                "' -> zones: " + std::to_string(zoneProbs[0]) + ":" +
                std::to_string(zoneProbs[1]) + ":" + std::to_string(zoneProbs[2]));
        }
    }

    file.close();
    Logger::Log("Spawn config loaded: " + std::to_string(m_spawnRules.size()) + " spawn tiles with rules");
    return true;
}

const SpawnRule* WorldSpawnConfig::GetSpawnRule(char spawnTile) const {
    auto it = m_spawnRules.find(spawnTile);
    if (it != m_spawnRules.end()) {
        return &it->second;
    }
    return nullptr;
}