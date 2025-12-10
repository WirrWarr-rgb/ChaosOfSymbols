#pragma once
#include <memory>
#include <functional>
#include <unordered_set>
#include "FileWatcher.h"
#include "TileTypeManager.h"
#include "FoodManager.h"
#include "CellularAutomatonRules.h"
#include "PlayerConfig.h"

class ConfigManager {
public:
    // Конструкто/деструктор
    ConfigManager();
    ~ConfigManager();

    // Публичные методы
    bool Initialize();
    void Update();

    // Геттеры
    TileTypeManager* GetTileManager() { return m_tileManager.get(); }
    FoodManager* GetFoodManager() { return m_foodManager.get(); }
    CellularAutomatonConfig* GetAutomatonConfig() { return m_automatonConfig.get(); }
    PlayerConfig* GetPlayerConfig() const { return m_playerConfig.get(); }

    // Сигналы для уведомления об изменениях
    std::function<void()> OnTilesChanged;
    std::function<void()> OnFoodChanged;
    std::function<void()> OnAutomatonRulesChanged;

    const std::vector<Food*>& GetAllFood() const { return m_foods; }
    int GetFoodCount() const { return static_cast<int>(m_foods.size()); }

private:
    std::vector<Food*> m_foods;
    std::unordered_map<int, Food*> m_foodMap;
    // Приватные методы
    void ReloadTiles();
    void ReloadFood();
    void ReloadAutomatonRules();

    // Приватные поля
    std::unique_ptr<FileWatcher> m_fileWatcher;
    std::unique_ptr<TileTypeManager> m_tileManager;
    std::unique_ptr<FoodManager> m_foodManager;
    std::unique_ptr<CellularAutomatonConfig> m_automatonConfig;
    std::unordered_set<int> m_previousTileIds;
    std::unordered_set<int> m_previousFoodIds;

    std::unique_ptr<PlayerConfig> m_playerConfig;

    bool m_initialized;
};