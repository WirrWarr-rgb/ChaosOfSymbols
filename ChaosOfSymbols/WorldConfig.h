#pragma once
#include "ConfigParser.h"
#include "SpawnRule.h"
#include <string>
#include <unordered_map>

class WorldConfig : public ConfigParser {
private:
    // Основные настройки генерации
    int m_width;
    int m_height;
    int m_seed;
    bool m_useRandomSeed;
    float m_noiseFrequency;
    int m_neighborRadius;

    // Правила спавна тайлов
    std::unordered_map<char, SpawnRule> m_spawnRules;

    // Пути к конфигурационным файлам
    std::string m_worldConfigPath;
    std::string m_spawnConfigPath;

protected:
    bool ParseKeyValue(const std::string& key, const std::string& value) override;
    bool ParseSpawnConfig();

public:
    WorldConfig();
    WorldConfig(const std::string& worldConfigPath, const std::string& spawnConfigPath);

    // Загрузка конфигурации
    bool LoadConfig();

    // Геттеры для основных параметров
    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }
    int GetSeed() const { return m_seed; }
    bool UseRandomSeed() const { return m_useRandomSeed; }
    float GetNoiseFrequency() const { return m_noiseFrequency; }

    // Получение эффективного сида (с учетом случайной генерации)
    int GetEffectiveSeed() const;

    // Работа с правилами спавна
    const SpawnRule* GetSpawnRule(char spawnTile) const;
    const std::unordered_map<char, SpawnRule>& GetAllSpawnRules() const { return m_spawnRules; }

    // Сеттеры
    void SetWidth(int width) { m_width = width; }
    void SetHeight(int height) { m_height = height; }
    void SetSeed(int seed) { m_seed = seed; }
    void SetUseRandomSeed(bool useRandom) { m_useRandomSeed = useRandom; }
    void SetNoiseFrequency(float frequency) { m_noiseFrequency = frequency; }

    void SetWorldConfigPath(const std::string& path) { m_worldConfigPath = path; }
    void SetSpawnConfigPath(const std::string& path) { m_spawnConfigPath = path; }

    int GetNeighborRadius() const { return m_neighborRadius; }
    void SetNeighborRadius(int radius) { m_neighborRadius = radius; }
};