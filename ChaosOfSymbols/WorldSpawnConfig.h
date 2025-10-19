#pragma once
#include "SpawnRule.h"
#include <vector>
#include <string>
#include <unordered_map>

class WorldSpawnConfig {
private:
    std::unordered_map<char, SpawnRule> m_spawnRules; // Изменено на один rule на символ
    std::string m_configPath;

public:
    WorldSpawnConfig(const std::string& configPath = "config/world_spawn.cfg");

    bool LoadFromFile();
    const SpawnRule* GetSpawnRule(char spawnTile) const;
    const std::unordered_map<char, SpawnRule>& GetAllRules() const { return m_spawnRules; }
};