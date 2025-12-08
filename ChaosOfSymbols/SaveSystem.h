#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <ctime>
#include <functional>
#include "SaveTypes.h"
#include "WorldConfig.h"

class SaveSystem {
public:
    SaveSystem();

    std::vector<SaveInfo> GetAllSaves();
    std::vector<SaveInfo> GetSaves(GameMode mode);
    bool CreateNewSave(GameMode mode, int slot, const std::string& name, const WorldConfig& config);
    bool LoadSave(GameMode mode, int slot);
    bool SaveGame(GameMode mode, int slot, const std::string& name = "");
    bool DeleteSave(GameMode mode, int slot);

    bool SaveWorldConfig(GameMode mode, int slot, const WorldConfig& config);
    WorldConfig LoadWorldConfig(GameMode mode, int slot);
    bool SavePlayerConfig(GameMode mode, int slot, const WorldConfig& config);
    bool SaveTilesConfig(GameMode mode, int slot, const WorldConfig& config);
    bool SaveAutomatonConfig(GameMode mode, int slot, const WorldConfig& config);

    bool LoadPlayerConfig(GameMode mode, int slot);
    bool LoadTilesConfig(GameMode mode, int slot);
    bool LoadAutomatonConfig(GameMode mode, int slot);

    const WorldConfig& GetLoadedConfig() const { return m_loadedConfig; }

    std::string GetSavesDirectory(GameMode mode) const;
    std::string GetSaveSlotPath(GameMode mode, int slot) const;
    bool IsSaveSlotEmpty(GameMode mode, int slot) const;
    SaveInfo GetSaveInfo(GameMode mode, int slot) const;

    SaveInfo GetSaveInfoUnified(int slot) const;

    bool SaveExistsAnywhere(int slot) const;

    int GetSelectedSlot() const { return m_selectedSlot; }
    void SetSelectedSlot(int slot) { m_selectedSlot = slot; }

private:
    void InitializeSaveDirectories();
    std::string GetCurrentDateTime() const;
    bool CopyDefaultConfigs(const std::string& sourceDir, const std::string& targetDir);
    bool SaveConfigToFile(const std::string& filePath, const std::string& content);
    bool LoadConfigFromFile(const std::string& filePath, std::function<void(const std::string&, const std::string&)> parser);

    WorldConfig m_loadedConfig;

    const std::string BASE_SAVES_DIR = "saves";
    const std::string PROCEDURAL_DIR = "proceduralGeneration";
    const std::string PRELOADED_DIR = "preloadedMaps";

    bool LoadSaveUnified(int slot);

    int m_selectedSlot = 1;
};