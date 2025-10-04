#pragma once
#include "FastNoiseLite.h"
#include "WorldGenerationConfig.h"
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

public:
    World();

    void Generate(int seed);
    void GenerateFromConfig(const std::string& configPath = "config/world_gen.cfg");
    bool LoadFromFile(const std::string& path);
    bool SaveToFile(const std::string& path);
    int GetTileAt(int x, int y) const;

    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }
    int GetCurrentSeed() const { return m_currentSeed; }
    const WorldGenConfig& GetGenConfig() const { return m_genConfig; }

    void SetSize(int width, int height);
};  