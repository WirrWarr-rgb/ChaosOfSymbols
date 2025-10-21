#pragma once
#include "FileWatcher.h"
#include "TileTypeManager.h"
#include "FoodManager.h"
#include "CellularAutomatonRules.h"
#include <memory>
#include <functional>
#include <unordered_set>

class ConfigManager {
public:
    ConfigManager();
    ~ConfigManager();

    bool Initialize();
    void Update();

    // Геттеры для менеджеров
    TileTypeManager* GetTileManager() { return m_tileManager.get(); }
    FoodManager* GetFoodManager() { return m_foodManager.get(); }
    CellularAutomatonConfig* GetAutomatonConfig() { return m_automatonConfig.get(); }

    // Сигналы для уведомления об изменениях
    std::function<void()> OnTilesChanged;
    std::function<void()> OnFoodChanged;
    std::function<void()> OnAutomatonRulesChanged;

private:
    void ReloadTiles();
    void ReloadFood();
    void ReloadAutomatonRules();

    std::unique_ptr<FileWatcher> m_fileWatcher;
    std::unique_ptr<TileTypeManager> m_tileManager;
    std::unique_ptr<FoodManager> m_foodManager;
    std::unique_ptr<CellularAutomatonConfig> m_automatonConfig;

    // Для отслеживания предыдущих состояний
    std::unordered_set<int> m_previousTileIds;
    std::unordered_set<int> m_previousFoodIds;

    bool m_initialized;
};