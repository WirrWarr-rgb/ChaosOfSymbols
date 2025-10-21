#pragma once
#include "FastNoiseLite.h"
#include "WorldConfig.h"
#include "CellularAutomatonRules.h"
#include "TileTypeManager.h"
#include "FoodManager.h"
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
    WorldConfig m_config;
    bool m_automatonEnabled;
    TileTypeManager* m_tileManager;
    FoodManager* m_foodManager;
    std::unordered_map<int, FoodSpawn> m_foodSpawns; // key: y * m_contentWidth + x

    // ИСПРАВЛЕНИЕ: используем указатель на внешний конфиг
    CellularAutomatonConfig* m_automatonConfig;

public:
    World();
    void GenerateFromConfig();
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

    int GetCurrentSeed() const { return m_config.GetEffectiveSeed(); }

    void SetTileManager(TileTypeManager* tileManager) { m_tileManager = tileManager; }
    void SetFoodManager(FoodManager* foodManager) { m_foodManager = foodManager; }

    // Методы для клеточного автомата
    void UpdateCellularAutomaton();
    void SetAutomatonEnabled(bool enabled) { m_automatonEnabled = enabled; }
    bool IsAutomatonEnabled() const { return m_automatonEnabled; }

    // Методы для еды
    void SpawnRandomFood(int count = 10);
    const Food* GetFoodAt(int x, int y) const;
    bool RemoveFoodAt(int x, int y);
    void RespawnFoodPeriodically();
    void ClearAllFood();

    // ИСПРАВЛЕНИЕ: правильные методы для работы с конфигом автомата
    void SetAutomatonConfig(CellularAutomatonConfig* config) {
        m_automatonConfig = config;
    }

    CellularAutomatonConfig* GetAutomatonConfig() const {
        return m_automatonConfig;
    }

    void NotifyTilesChanged() {}

    void UpdateTileAppearance(); // Обновляет внешний вид тайлов
    void RemoveDeletedTiles(const std::unordered_set<int>& removedTileIds);

private:
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

    // Вспомогательные методы для еды
    bool CanSpawnFoodAt(int x, int y) const;
    int GetRandomPassablePosition(int& outX, int& outY);
    void CheckNeighbor(int x, int y, int dx, int dy,
        const std::vector<std::vector<int>>& currentMap,
        std::unordered_map<char, int>& counts) const;
};