#pragma once
#include "TileType.h"
#include <unordered_map>
#include <string>

class TileTypeManager {
private:
    std::unordered_map<int, TileType> m_tileTypes;
    std::string m_resourceFilePath;

    void LoadDefaultTiles();

public:
    TileTypeManager(const std::string& filePath = "config/tiles.json");

    bool LoadFromFile();
    bool LoadFromFile(const std::string& filePath);
    bool SaveToFile(const std::string& filePath = "");
    TileType* GetTileType(int id);
    void RegisterTileType(const TileType& tileType);
    size_t GetTileCount() const { return m_tileTypes.size(); }

    const std::unordered_map<int, TileType>& GetAllTiles() const { return m_tileTypes; }
};