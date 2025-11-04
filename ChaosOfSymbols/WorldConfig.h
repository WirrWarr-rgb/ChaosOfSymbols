#pragma once
#include <string>
#include <unordered_map>
#include "ConfigParser.h"
#include "SpawnRule.h"

// Режимы генерации мира
enum class WorldGenerationMode {
    RANDOM,         // Случайная генерация (сид генерируется автоматически)
    SEEDED,         // Генерация с сидом из файла
    FROM_MAP_FILE   // Без генерации, карта из файла
};

class WorldConfig : public ConfigParser {
public:
    WorldConfig();
    WorldConfig(const std::string& worldConfigPath, const std::string& spawnConfigPath);

    // Публичные методы
    bool LoadConfig();

    // Геттеры
    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }
    int GetSeed() const { return m_seed; }
    float GetNoiseFrequency() const { return m_noiseFrequency; }
    int GetEffectiveSeed() const;
    const SpawnRule* GetSpawnRule(char spawnTile) const;
    const std::unordered_map<char, SpawnRule>& GetAllSpawnRules() const { return m_spawnRules; }
    int GetNeighborRadius() const { return m_neighborRadius; }
    WorldGenerationMode GetGenerationMode() const { return m_generationMode; }
    const std::string& GetMapFilePath() const { return m_mapFilePath; }

    // Сеттеры
    void SetWidth(int width) { m_width = width; }
    void SetHeight(int height) { m_height = height; }
    void SetSeed(int seed) { m_seed = seed; }
    void SetNoiseFrequency(float frequency) { m_noiseFrequency = frequency; }
    void SetWorldConfigPath(const std::string& path) { m_worldConfigPath = path; }
    void SetSpawnConfigPath(const std::string& path) { m_spawnConfigPath = path; }
    void SetNeighborRadius(int radius) { m_neighborRadius = radius; }
    void SetGenerationMode(WorldGenerationMode mode) { m_generationMode = mode; }
    void SetMapFilePath(const std::string& path) { m_mapFilePath = path; }

protected:
    bool ParseKeyValue(const std::string& key, const std::string& value) override;
    bool ParseSpawnConfig();

private:
    int m_width;
    int m_height;
    int m_seed;
    float m_noiseFrequency;
    int m_neighborRadius;
    WorldGenerationMode m_generationMode;
    std::string m_mapFilePath;

    std::unordered_map<char, SpawnRule> m_spawnRules;

    std::string m_worldConfigPath;
    std::string m_spawnConfigPath;
};