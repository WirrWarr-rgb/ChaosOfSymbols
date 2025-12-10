#include "ConfigManager.h"
#include "Logger.h"
#include <filesystem>

namespace fs = std::filesystem;

ConfigManager::ConfigManager(const std::string& saveSlotPath)
    : m_initialized(false), m_saveSlotPath(saveSlotPath) {
    m_fileWatcher = std::make_unique<FileWatcher>();
}

ConfigManager::~ConfigManager() {
    if (m_fileWatcher) {
        m_fileWatcher->Stop();
    }
}

/// <summary>
/// Инициализирует все подсистемы конфигураций и запускает наблюдение за файлами
/// </summary>
bool ConfigManager::Initialize() {
    Logger::Log("Initializing ConfigManager from save slot: " + m_saveSlotPath);

    std::string playerConfigPath = m_saveSlotPath + "player.cfg";
    std::string tilesPath = m_saveSlotPath + "tiles.json";
    std::string foodPath = m_saveSlotPath + "food.cfg";
    std::string automatonPath = m_saveSlotPath + "cellular_automaton.cfg";

    Logger::Log("Config paths:");
    Logger::Log("  Tiles: " + tilesPath);
    Logger::Log("  Food: " + foodPath);
    Logger::Log("  Automaton: " + automatonPath);

    if (!fs::exists(tilesPath)) {
        Logger::Log("ERROR: Tiles file not found: " + tilesPath);
        return false;
    }
    if (!fs::exists(automatonPath)) {
        Logger::Log("ERROR: Automaton file not found: " + automatonPath);
        return false;
    }

    m_playerConfig = std::make_unique<PlayerConfig>(playerConfigPath);
    if (!m_playerConfig->LoadConfig()) {
        Logger::Log("ERROR: Failed to load player config from save");
        return false;
    }

    m_tileManager = std::make_unique<TileTypeManager>(tilesPath);
    m_foodManager = std::make_unique<FoodManager>(foodPath);
    m_automatonConfig = std::make_unique<CellularAutomatonConfig>();

    if (!m_tileManager->LoadFromFile()) {
        Logger::Log("ERROR: Failed to load tiles from save");
        return false;
    }

    if (!m_foodManager->LoadFromFile()) {
        Logger::Log("ERROR: Failed to load food config from save " + foodPath);
        return false;
    }

    if (!m_automatonConfig->LoadFromFile(automatonPath)) {
        Logger::Log("WARNING: Failed to load automaton config from save");
    }

    m_fileWatcher->WatchFile(tilesPath,
        [this]() { this->ReloadTiles(); });

    m_fileWatcher->WatchFile(foodPath,
        [this]() { this->ReloadFood(); });

    m_fileWatcher->WatchFile(automatonPath,
        [this]() { this->ReloadAutomatonRules(); });

    m_initialized = true;
    Logger::Log("ConfigManager initialized successfully from save slot");
    return true;
}

/// <summary>
/// Обновляет состояние всех наблюдателей файлов (вызывается каждый кадр)
/// </summary>
void ConfigManager::Update() {
    if (m_fileWatcher) {
        m_fileWatcher->Update();
    }
}

/// <summary>
///  Перезагрузка конфигурации тайлов при изменении файла
/// </summary>
void ConfigManager::ReloadTiles() {
    Logger::Log("Reloading tile configurations...");

    std::unordered_set<int> currentTileIds;
    const auto& currentTiles = m_tileManager->GetAllTiles();
    for (const auto& pair : currentTiles) {
        currentTileIds.insert(pair.first);
    }

    if (m_tileManager->LoadFromFile()) {
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

/// <summary>
/// Перезагрузка конфигурации еды при изменении файла
/// </summary>
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

/// <summary>
/// Перезагрузка конфигурации правил клеточного автомата при изменении файла
/// </summary>
void ConfigManager::ReloadAutomatonRules() {
    Logger::Log("Reloading cellular automaton rules...");

    std::string automatonPath = m_saveSlotPath + "cellular_automaton.cfg";

    if (m_automatonConfig->LoadFromFile(automatonPath)) {
        Logger::Log("Cellular automaton rules reloaded successfully from save");
        if (OnAutomatonRulesChanged) {
            OnAutomatonRulesChanged();
        }
    }
    else {
        Logger::Log("ERROR: Failed to reload cellular automaton rules from save");
    }
}