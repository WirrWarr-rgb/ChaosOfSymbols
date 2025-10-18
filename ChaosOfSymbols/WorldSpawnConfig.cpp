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
        if (line.empty() || line[0] == '#') continue;

        std::stringstream ss(line);
        std::string spawnTileStr, probabilityStr, allowedTilesStr;

        // ����� ������: spawnTile=probability:allowedBaseTiles
        // �������: 
        // T=0.3:.       - ������ ��������� �� ����� � ������������ 30%
        // ~=0.1:.~      - ���� ��������� �� ����� ��� ���� � ������������ 10%
        // ,=0.02:*      - ����� ��������� ��� ������ � ������������ 2%

        if (std::getline(ss, spawnTileStr, '=') &&
            std::getline(ss, probabilityStr, ':') &&
            std::getline(ss, allowedTilesStr)) {

            if (spawnTileStr.length() != 1) {
                Logger::Log("WARNING: Invalid spawn tile in config: " + spawnTileStr);
                continue;
            }

            char spawnTile = spawnTileStr[0];
            float probability = std::stof(probabilityStr);

            SpawnRule rule;
            rule.tileId = -1; // ����� ���������� ����� �� �������
            rule.probability = probability;

            // ������������ ����������� ������� �����
            for (char allowedTile : allowedTilesStr) {
                if (allowedTile != ',') {
                    rule.allowedBaseTiles.insert(allowedTile);
                }
            }

            m_spawnRules[spawnTile].push_back(rule);
            Logger::Log("Added spawn rule: '" + std::string(1, spawnTile) +
                "' -> prob: " + std::to_string(probability) +
                ", allowed: '" + allowedTilesStr + "'");
        }
    }

    file.close();
    Logger::Log("Spawn config loaded: " + std::to_string(m_spawnRules.size()) + " spawn tiles with rules");
    return true;
}

const std::vector<SpawnRule>& WorldSpawnConfig::GetSpawnRules(char spawnTile) const {
    static const std::vector<SpawnRule> empty;
    auto it = m_spawnRules.find(spawnTile);
    return (it != m_spawnRules.end()) ? it->second : empty;
}