#pragma once
#include <memory>
#include <string>
#include <unordered_set>
#include <functional>
#include "PlayerConfig.h"
#include "TileTypeManager.h"
#include "FoodManager.h"
#include "CellularAutomatonRules.h"
#include "FileWatcher.h"

class ConfigManager {
public:
    // Конструкторы
    ConfigManager(const std::string& saveSlotPath);
    ~ConfigManager();

    // Методы
    bool Initialize();
    void Update();

    // Геттеры
    PlayerConfig* GetPlayerConfig() const { return m_playerConfig.get(); }
    TileTypeManager* GetTileManager() const { return m_tileManager.get(); }
    FoodManager* GetFoodManager() const { return m_foodManager.get(); }
    CellularAutomatonConfig* GetAutomatonConfig() const { return m_automatonConfig.get(); }

    // Коллбеки
    std::function<void()> OnTilesChanged;
    std::function<void()> OnFoodChanged;
    std::function<void()> OnAutomatonRulesChanged;

private:
    // Приватные методы
    void ReloadTiles();
    void ReloadFood();
    void ReloadAutomatonRules();

    // Приватные поля
    bool m_initialized;
    std::string m_saveSlotPath;
    std::unique_ptr<FileWatcher> m_fileWatcher;
    std::unique_ptr<PlayerConfig> m_playerConfig;
    std::unique_ptr<TileTypeManager> m_tileManager;
    std::unique_ptr<FoodManager> m_foodManager;
    std::unique_ptr<CellularAutomatonConfig> m_automatonConfig;
    std::unordered_set<int> m_previousTileIds;
    std::unordered_set<int> m_previousFoodIds;
};