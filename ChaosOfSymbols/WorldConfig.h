#pragma once
#include <string>
#include <unordered_map>
#include "ConfigParser.h"
#include "SpawnRule.h"

class WorldConfig : public ConfigParser {
public:
    WorldConfig();
    WorldConfig(const std::string& worldConfigPath, const std::string& spawnConfigPath);

    // Пцбличные методы
    bool LoadConfig();

    // Геттеры
    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }
    int GetSeed() const { return m_seed; }
    bool UseRandomSeed() const { return m_useRandomSeed; }
    float GetNoiseFrequency() const { return m_noiseFrequency; }
    int GetEffectiveSeed() const;
    const SpawnRule* GetSpawnRule(char spawnTile) const;
    const std::unordered_map<char, SpawnRule>& GetAllSpawnRules() const { return m_spawnRules; }
    int GetNeighborRadius() const { return m_neighborRadius; }

    // Сеттеры
    void SetWidth(int width) { m_width = width; }
    void SetHeight(int height) { m_height = height; }
    void SetSeed(int seed) { m_seed = seed; }
    void SetUseRandomSeed(bool useRandom) { m_useRandomSeed = useRandom; }
    void SetNoiseFrequency(float frequency) { m_noiseFrequency = frequency; }
    void SetWorldConfigPath(const std::string& path) { m_worldConfigPath = path; }
    void SetSpawnConfigPath(const std::string& path) { m_spawnConfigPath = path; }
    void SetNeighborRadius(int radius) { m_neighborRadius = radius; }

protected:
    bool ParseKeyValue(const std::string& key, const std::string& value) override;
    bool ParseSpawnConfig();

private:
    int m_width;
    int m_height;
    int m_seed;
    bool m_useRandomSeed;
    float m_noiseFrequency;
    int m_neighborRadius;

    std::unordered_map<char, SpawnRule> m_spawnRules;

    std::string m_worldConfigPath;
    std::string m_spawnConfigPath;
};