#include "ConfigManager.h"
#include "Logger.h"

ConfigManager::ConfigManager() : m_initialized(false) {
    m_fileWatcher = std::make_unique<FileWatcher>();
}

ConfigManager::~ConfigManager() {
    if (m_fileWatcher) {
        m_fileWatcher->Stop();
    }
}

bool ConfigManager::Initialize() {
    Logger::Log("Initializing ConfigManager\n");

    m_tileManager = std::make_unique<TileTypeManager>();
    m_foodManager = std::make_unique<FoodManager>();
    m_automatonConfig = std::make_unique<CellularAutomatonConfig>();

    if (!m_tileManager->LoadFromFile()) {
        Logger::Log("ERROR: Failed to load initial tile config");
        return false;
    }

    const auto& initialTiles = m_tileManager->GetAllTiles();
    for (const auto& pair : initialTiles) {
        m_previousTileIds.insert(pair.first);
    }

    if (!m_foodManager->LoadFromFile()) {
        Logger::Log("WARNING: Failed to load initial food config");
    }
    else {
        for (int i = 0; i < m_foodManager->GetFoodCount(); i++) {
            if (const Food* food = m_foodManager->GetFood(i)) {
                m_previousFoodIds.insert(food->GetId());
            }
        }
    }

    if (!m_automatonConfig->LoadFromFile("config/cellular_automaton.cfg")) {
        Logger::Log("WARNING: Failed to load initial automaton config");
    }

    m_fileWatcher->WatchFile("config/tiles.json",
        [this]() { this->ReloadTiles(); });

    m_fileWatcher->WatchFile("config/food.cfg",
        [this]() { this->ReloadFood(); });

    m_fileWatcher->WatchFile("config/cellular_automaton.cfg",
        [this]() { this->ReloadAutomatonRules(); });

    m_initialized = true;
    Logger::Log("\nConfigManager initialized successfully");
    return true;
}

void ConfigManager::Update() {
    if (m_fileWatcher) {
        m_fileWatcher->Update();
    }
}

void ConfigManager::ReloadTiles() {
    Logger::Log("Reloading tile configurations...");

    // Сохраняем текущие ID перед перезагрузкой
    std::unordered_set<int> currentTileIds;
    const auto& currentTiles = m_tileManager->GetAllTiles();
    for (const auto& pair : currentTiles) {
        currentTileIds.insert(pair.first);
    }

    if (m_tileManager->LoadFromFile()) {
        // Определяем, какие тайлы были удалены
        std::unordered_set<int> newTileIds;
        const auto& newTiles = m_tileManager->GetAllTiles();
        for (const auto& pair : newTiles) {
            newTileIds.insert(pair.first);
        }

        std::unordered_set<int> removedTiles;
        for (int id : currentTileIds) {
            if (newTileIds.find(id) == newTileIds.end()) {
                removedTiles.insert(id);
            }
        }

        if (!removedTiles.empty()) {
            Logger::Log("Tiles removed: " + std::to_string(removedTiles.size()));
        }

        Logger::Log("Tile configurations reloaded successfully. Total tiles: " +
            std::to_string(newTileIds.size()));

        m_previousTileIds = newTileIds;

        if (OnTilesChanged) {
            OnTilesChanged();
        }
    }
    else {
        Logger::Log("ERROR: Failed to reload tile configurations");
    }
}

void ConfigManager::ReloadFood() {
    Logger::Log("Reloading food configurations...");

    if (m_foodManager->LoadFromFile()) {
        Logger::Log("Food configurations reloaded successfully");
        if (OnFoodChanged) {
            OnFoodChanged();
        }
    }
    else {
        Logger::Log("ERROR: Failed to reload food configurations");
    }
}

void ConfigManager::ReloadAutomatonRules() {
    Logger::Log("Reloading cellular automaton rules...");

    if (m_automatonConfig->LoadFromFile("config/cellular_automaton.cfg")) {
        Logger::Log("Cellular automaton rules reloaded successfully");
        if (OnAutomatonRulesChanged) {
            OnAutomatonRulesChanged();
        }
    }
    else {
        Logger::Log("ERROR: Failed to reload cellular automaton rules");
    }
}