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
    bool CreateNewSave(int slot, const std::string& name, const WorldConfig& config);
    bool LoadSave(int slot);
    bool SaveGame(int slot, const std::string& name = "");
    bool DeleteSave(int slot);

    bool SaveWorldConfig(int slot, const WorldConfig& config);
    WorldConfig LoadWorldConfig(int slot);

    // Добавляем недостающие методы:
    bool SavePlayerConfig(int slot, const WorldConfig& config);
    bool SaveTilesConfig(int slot, const WorldConfig& config);
    bool SaveAutomatonConfig(int slot, const WorldConfig& config);

    // Эти методы нужны для загрузки
    bool LoadPlayerConfig(int slot);
    bool LoadTilesConfig(int slot);
    bool LoadAutomatonConfig(int slot);

    SaveInfo GetSaveInfo(GameMode mode, int slot) const;  // Для совместимости
    SaveInfo GetSaveInfo(int slot) const;                 // Основной метод
    SaveInfo GetSaveInfoUnified(int slot) const;          // Для совместимости
    bool SaveExistsAnywhere(int slot) const;              // Для совместимости

    std::string GetSaveSlotPath(int slot) const;
    bool IsSaveSlotEmpty(int slot) const;

    const WorldConfig& GetLoadedConfig() const { return m_loadedConfig; }

    int GetSelectedSlot() const { return m_selectedSlot; }
    void SetSelectedSlot(int slot) { m_selectedSlot = slot; }
    std::string GetCurrentDateTime() const;
    bool CopyTemplateToSave(int templateSlot, int saveSlot);

private:
    void InitializeSaveDirectories();
    bool SaveConfigToFile(const std::string& filePath, const std::string& content);
    bool LoadConfigFromFile(const std::string& filePath,
        std::function<void(const std::string&, const std::string&)> parser);

    bool LoadSaveUnified(int slot);

    WorldConfig m_loadedConfig;

    const std::string BASE_SAVES_DIR = "saves";

    int m_selectedSlot = 1;
};