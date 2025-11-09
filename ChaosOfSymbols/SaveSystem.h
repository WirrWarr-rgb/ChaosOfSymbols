#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <ctime>
#include <functional>
#include "SaveTypes.h"
#include "WorldEditorConfig.h"

class SaveSystem {
public:
    SaveSystem();

    // Основные методы
    std::vector<SaveInfo> GetSaves(GameMode mode);
    bool CreateNewSave(GameMode mode, int slot, const std::string& name, const WorldEditorConfig& config);
    bool LoadSave(GameMode mode, int slot);
    bool SaveGame(GameMode mode, int slot, const std::string& name = "");
    bool DeleteSave(GameMode mode, int slot);

    // Методы для работы с конфигами редактора
    bool SaveWorldConfig(GameMode mode, int slot, const WorldEditorConfig& config);
    WorldEditorConfig LoadWorldConfig(GameMode mode, int slot);
    bool SavePlayerConfig(GameMode mode, int slot, const WorldEditorConfig& config);
    bool SaveTilesConfig(GameMode mode, int slot, const WorldEditorConfig& config);
    bool SaveAutomatonConfig(GameMode mode, int slot, const WorldEditorConfig& config);

    // Новые методы для загрузки конфигов
    bool LoadPlayerConfig(GameMode mode, int slot);
    bool LoadTilesConfig(GameMode mode, int slot);
    bool LoadAutomatonConfig(GameMode mode, int slot);

    // Геттер для загруженной конфигурации
    const WorldEditorConfig& GetLoadedConfig() const { return m_loadedConfig; }

    // Вспомогательные методы
    std::string GetSavesDirectory(GameMode mode) const;
    std::string GetSaveSlotPath(GameMode mode, int slot) const;
    bool IsSaveSlotEmpty(GameMode mode, int slot) const;
    SaveInfo GetSaveInfo(GameMode mode, int slot) const;

    int GetSelectedSlot() const { return m_selectedSlot; }
    void SetSelectedSlot(int slot) { m_selectedSlot = slot; }

private:
    void InitializeSaveDirectories();
    std::string GetCurrentDateTime() const;
    bool CopyDefaultConfigs(const std::string& sourceDir, const std::string& targetDir);
    bool SaveConfigToFile(const std::string& filePath, const std::string& content);
    bool LoadConfigFromFile(const std::string& filePath, std::function<void(const std::string&, const std::string&)> parser);

    WorldEditorConfig m_loadedConfig; // Загруженная конфигурация

    const std::string BASE_SAVES_DIR = "saves";
    const std::string PROCEDURAL_DIR = "proceduralGeneration";
    const std::string PRELOADED_DIR = "preloadedMaps";

    int m_selectedSlot = 1;
};