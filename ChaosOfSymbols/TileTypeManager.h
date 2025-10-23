#pragma once
#include <unordered_map>
#include <string>
#include "TileType.h"

class TileTypeManager {
public:
    TileTypeManager(const std::string& filePath = "config/tiles.json");

    bool LoadFromFile();
    bool LoadFromFile(const std::string& filePath);
    bool SaveToFile(const std::string& filePath = "");
    void RegisterTileType(const TileType& tileType);

    // Геттеры
    const std::unordered_map<int, TileType>& GetAllTiles() const { return m_tileTypes; }
    TileType* GetTileType(int id);
    size_t GetTileCount() const { return m_tileTypes.size(); }

private:
    void LoadDefaultTiles();

    std::unordered_map<int, TileType> m_tileTypes;
    std::string m_resourceFilePath;
};