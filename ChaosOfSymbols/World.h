#pragma once
#include "FastNoiseLite.h"
#include "WorldGenerationConfig.h"
#include "WorldSpawnConfig.h"
#include "CellularAutomatonRules.h"
#include "TileTypeManager.h"
#include <vector>
#include <string>

class World {
private:
    std::vector<std::vector<int>> m_map;
    int m_width;
    int m_height;
    FastNoiseLite m_noiseGenerator;
    int m_currentSeed;
    WorldGenConfig m_genConfig;
    WorldSpawnConfig m_spawnConfig;
    CellularAutomatonConfig m_automatonConfig;
    bool m_automatonEnabled;
    TileTypeManager* m_tileManager;

public:
    World();
    void GenerateFromConfig(const std::string& configPath = "config/world_gen.cfg");
    int GetTileAt(int x, int y) const;

    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }
    int GetCurrentSeed() const { return m_currentSeed; }

    void SetTileManager(TileTypeManager* tileManager) { m_tileManager = tileManager; }

    // Методы для клеточного автомата
    void UpdateCellularAutomaton();
    void SetAutomatonEnabled(bool enabled) { m_automatonEnabled = enabled; }
    bool IsAutomatonEnabled() const { return m_automatonEnabled; }

private:
    void GenerateBaseTerrain();
    void ApplySpawnRules();
    char GetTileCharacter(int tileId) const;
    int FindTileIdByCharacter(char character) const;
    std::unordered_map<char, int> CountNeighbors(int x, int y, const std::vector<std::vector<int>>& currentMap) const;
};