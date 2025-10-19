#pragma once
#include "FastNoiseLite.h"
#include "WorldGenerationConfig.h"
#include "WorldSpawnConfig.h"
#include "CellularAutomatonRules.h"
#include "TileTypeManager.h"
#include "FoodManager.h" // ДОБАВЬТЕ ЭТО
#include <vector>
#include <string>
#include <unordered_map>

struct FoodSpawn {
    int x, y;
    int foodId;
};

class World {
private:
    std::vector<std::vector<int>> m_map;
    int m_width;
    int m_height;
    int m_contentWidth;
    int m_contentHeight;
    FastNoiseLite m_noiseGenerator;
    int m_currentSeed;
    WorldGenConfig m_genConfig;
    WorldSpawnConfig m_spawnConfig;
    CellularAutomatonConfig m_automatonConfig;
    bool m_automatonEnabled;
    TileTypeManager* m_tileManager;
    FoodManager* m_foodManager; // ДОБАВЬТЕ ЭТО
    std::unordered_map<int, FoodSpawn> m_foodSpawns; // ДОБАВЬТЕ ЭТО - key: y * m_contentWidth + x

public:
    World();
    void GenerateFromConfig(const std::string& configPath = "config/world_gen.cfg");
    int GetTileAt(int x, int y) const;

    // Возвращаем размеры БЕЗ границы (игровое пространство)
    int GetWidth() const { return m_contentWidth; }
    int GetHeight() const { return m_contentHeight; }

    // Методы для получения полных размеров
    int GetTotalWidth() const { return m_width; }
    int GetTotalHeight() const { return m_height; }

    // Метод для получения тайла из полной карты (с границей)
    int GetTileAtFullMap(int x, int y) const {
        if (x >= 0 && x < m_width && y >= 0 && y < m_height) {
            return m_map[y][x];
        }
        return 0;
    }

    int GetCurrentSeed() const { return m_currentSeed; }

    void SetTileManager(TileTypeManager* tileManager) { m_tileManager = tileManager; }
    void SetFoodManager(FoodManager* foodManager) { m_foodManager = foodManager; } // ДОБАВЬТЕ ЭТО

    // Методы для клеточного автомата
    void UpdateCellularAutomaton();
    void SetAutomatonEnabled(bool enabled) { m_automatonEnabled = enabled; }
    bool IsAutomatonEnabled() const { return m_automatonEnabled; }

    // МЕТОДЫ ДЛЯ ЕДЫ - ДОБАВЬТЕ ЭТИ МЕТОДЫ
    void SpawnRandomFood(int count = 10);
    const Food* GetFoodAt(int x, int y) const;
    bool RemoveFoodAt(int x, int y);
    void RespawnFoodPeriodically(); // Для периодического респавна

    void ClearAllFood();
    void ClearScreen();

private:
    void GenerateBaseTerrain();
    void ApplySpawnRules();
    void CreateBorder();
    void SmoothTerrain();
    char GetTileCharacter(int tileId) const;
    int FindTileIdByCharacter(char character) const;
    std::unordered_map<char, int> CountNeighbors(int x, int y, const std::vector<std::vector<int>>& currentMap) const;
    char SelectTileByZone(int zone, const std::unordered_map<char, SpawnRule>& spawnRules, int x, int y);
    float GetProbabilityForZone(const SpawnRule& rule, int zone);
    char FindWaterTile(const std::unordered_map<char, SpawnRule>& spawnRules) const;
    char FindGrassTile(const std::unordered_map<char, SpawnRule>& spawnRules) const;
    char FindMountainTile(const std::unordered_map<char, SpawnRule>& spawnRules) const;
    char FindTileByTerrainType(const std::string& terrainType, const std::unordered_map<char, SpawnRule>& spawnRules) const;

    // ВСПОМОГАТЕЛЬНЫЕ МЕТОДЫ ДЛЯ ЕДЫ
    bool CanSpawnFoodAt(int x, int y) const;
    int GetRandomPassablePosition(int& outX, int& outY);
};