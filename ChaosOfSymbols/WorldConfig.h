#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include "ConfigParser.h"
#include "SpawnRule.h"
#include "TileTypeManager.h"

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
    bool LoadConfig(bool forceReload = false);

    // Методы для сохранения/загрузки
    bool SaveToDirectory(const std::string& directory) const;
    bool LoadFromDirectory(const std::string& directory);

    // Методы для конвертации в/из WorldEditorConfig
    void FromEditorConfig(const WorldConfig& editorConfig);
    WorldConfig ToEditorConfig() const;

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

    // Геттеры для редактора
    const std::string& GetWorldName() const { return m_worldName; }
    bool GetRandomGeneration() const { return m_generationMode == WorldGenerationMode::RANDOM; }
    int GetPlayerStartX() const { return m_playerStartX; }
    int GetPlayerStartY() const { return m_playerStartY; }
    int GetPlayerMaxHP() const { return m_playerMaxHP; }
    int GetPlayerMaxHunger() const { return m_playerMaxHunger; }
    bool GetEnableHP() const { return m_enableHP; }
    bool GetEnableHunger() const { return m_enableHunger; }
    const std::unordered_map<char, std::vector<float>>& GetTileProbabilities() const { return m_tileProbabilities; }
    const std::string& GetSurvivalRules() const { return m_survivalRules; }
    const std::string& GetBirthRules() const { return m_birthRules; }
    const std::string& GetDeathRules() const { return m_deathRules; }
    bool GetEnableEnemies() const { return m_enableEnemies; }
    float GetEnemySpawnRate() const { return m_enemySpawnRate; }

    // Проверки
    bool IsLoadedFromSave() const { return m_parametersLoadedFromSave; }

    // Сеттеры
    void SetWidth(int width) { m_width = width; }
    void SetHeight(int height) { m_height = height; }
    void SetSeed(int seed) { m_seed = seed; }
    void SetNoiseFrequency(float frequency) { m_noiseFrequency = frequency; }
    void SetWorldConfigPath(const std::string& path) { m_worldConfigPath = path; }
    void SetNeighborRadius(int radius) { m_neighborRadius = radius; }
    void SetGenerationMode(WorldGenerationMode mode) { m_generationMode = mode; }
    void SetMapFilePath(const std::string& path) { m_mapFilePath = path; }

    // Сеттеры для редактора
    void SetWorldName(const std::string& name) { m_worldName = name; }
    void SetRandomGeneration(bool random) {
        m_generationMode = random ? WorldGenerationMode::RANDOM : WorldGenerationMode::SEEDED;
    }
    void SetPlayerStartX(int x) { m_playerStartX = x; }
    void SetPlayerStartY(int y) { m_playerStartY = y; }
    void SetPlayerMaxHP(int hp) { m_playerMaxHP = hp; }
    void SetPlayerMaxHunger(int hunger) { m_playerMaxHunger = hunger; }
    void SetEnableHP(bool enable) { m_enableHP = enable; }
    void SetEnableHunger(bool enable) { m_enableHunger = enable; }
    void SetTileProbabilities(const std::unordered_map<char, std::vector<float>>& probs) {
        m_tileProbabilities = probs;
    }
    void SetSurvivalRules(const std::string& rules) { m_survivalRules = rules; }
    void SetBirthRules(const std::string& rules) { m_birthRules = rules; }
    void SetDeathRules(const std::string& rules) { m_deathRules = rules; }
    void SetEnableEnemies(bool enable) { m_enableEnemies = enable; }
    void SetEnemySpawnRate(float rate) { m_enemySpawnRate = rate; }

    void MarkAsLoadedFromSave() { m_parametersLoadedFromSave = true; }
    void ClearSaveMark() { m_parametersLoadedFromSave = false; }
    void CalculateSpawnRulesFromTiles(TileTypeManager* tileManager);
private:
    bool ParseKeyValue(const std::string& key, const std::string& value) override;

    // Основные параметры мира
    int m_width;
    int m_height;
    int m_seed;
    float m_noiseFrequency;
    int m_neighborRadius;
    WorldGenerationMode m_generationMode;
    std::string m_mapFilePath;

    // Параметры редактора
    std::string m_worldName;
    int m_playerStartX;
    int m_playerStartY;
    int m_playerMaxHP;
    int m_playerMaxHunger;
    bool m_enableHP;
    bool m_enableHunger;
    std::unordered_map<char, std::vector<float>> m_tileProbabilities;
    std::string m_survivalRules;
    std::string m_birthRules;
    std::string m_deathRules;
    bool m_enableEnemies;
    float m_enemySpawnRate;

    std::unordered_map<char, SpawnRule> m_spawnRules;

    std::string m_worldConfigPath;

    bool m_parametersLoadedFromSave = false;
};