#pragma once
#include "SpawnRule.h"
#include <vector>
#include <string>
#include <unordered_map>

class WorldSpawnConfig {
private:
    std::unordered_map<char, std::vector<SpawnRule>> m_spawnRules;
    std::string m_configPath;

public:
    WorldSpawnConfig(const std::string& configPath = "config/world_spawn.cfg");

    bool LoadFromFile();
    const std::vector<SpawnRule>& GetSpawnRules(char spawnTile) const;
    const std::unordered_map<char, std::vector<SpawnRule>>& GetAllRules() const { return m_spawnRules; }
};