#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include "FastNoiseLite.h"
#include "WorldConfig.h"
#include "CellularAutomatonRules.h"
#include "TileTypeManager.h"
#include "FoodManager.h"

struct FoodSpawn {
    int x, y;
    int foodId;
};

class World {
public:
    // Конструктор
    World();

    // Публичные методы
    void GenerateFromConfig();
    void UpdateTileAppearance();
    void UpdateCellularAutomaton();
    void RemoveDeletedTiles(const std::unordered_set<int>& removedTileIds);
    void SpawnRandomFood(int count = 10);
    void RespawnFoodPeriodically();
    void ClearAllFood();
    void NotifyTilesChanged() {}

    // Геттеры
    int GetTileAt(int x, int y) const;
    int GetWidth() const { return m_contentWidth; }
    int GetHeight() const { return m_contentHeight; }
    int GetTotalWidth() const { return m_width; }
    int GetTotalHeight() const { return m_height; }
    int GetTileAtFullMap(int x, int y) const {
        if (x >= 0 && x < m_width && y >= 0 && y < m_height) {
            return m_map[y][x];
        }
        return 0;
    }
    int GetCurrentSeed() const { return m_config.GetEffectiveSeed(); }
    bool IsAutomatonEnabled() const { return m_automatonEnabled; }
    const Food* GetFoodAt(int x, int y) const;
    CellularAutomatonConfig* GetAutomatonConfig() const {
        return m_automatonConfig;
    }

    // Сеттеры
    void SetTileManager(TileTypeManager* tileManager) { m_tileManager = tileManager; }
    void SetFoodManager(FoodManager* foodManager) { m_foodManager = foodManager; }
    void SetAutomatonEnabled(bool enabled) { m_automatonEnabled = enabled; }
    void SetAutomatonConfig(CellularAutomatonConfig* config) {
        m_automatonConfig = config;
    }
    bool RemoveFoodAt(int x, int y);

private:
    // Приватные методы
    void GenerateBaseTerrain();
    void CreateBorder();
    void SmoothTerrain();
    char GetTileCharacter(int tileId) const;
    int FindTileIdByCharacter(char character) const;
    std::unordered_map<char, int> CountNeighbors(int x, int y, const std::vector<std::vector<int>>& currentMap) const;
    char SelectTileByZone(int zone, const std::unordered_map<char, SpawnRule>& spawnRules, int x, int y);
    char FindWaterTile(const std::unordered_map<char, SpawnRule>& spawnRules) const;
    char FindGrassTile(const std::unordered_map<char, SpawnRule>& spawnRules) const;
    char FindMountainTile(const std::unordered_map<char, SpawnRule>& spawnRules) const;
    char FindTileByTerrainType(const std::string& terrainType, const std::unordered_map<char, SpawnRule>& spawnRules) const;
    bool CanSpawnFoodAt(int x, int y) const;
    int GetRandomPassablePosition(int& outX, int& outY);
    void CheckNeighbor(int x, int y, int dx, int dy,
        const std::vector<std::vector<int>>& currentMap,
        std::unordered_map<char, int>& counts) const;

    // Приватные поля
    std::vector<std::vector<int>> m_map;
    int m_width;
    int m_height;
    int m_contentWidth;
    int m_contentHeight;
    FastNoiseLite m_noiseGenerator;
    WorldConfig m_config;
    bool m_automatonEnabled;
    TileTypeManager* m_tileManager;
    FoodManager* m_foodManager;
    std::unordered_map<int, FoodSpawn> m_foodSpawns;
    CellularAutomatonConfig* m_automatonConfig;
};