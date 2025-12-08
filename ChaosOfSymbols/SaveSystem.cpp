#include "SaveSystem.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <sstream>
#include <algorithm>
#include "Logger.h"

namespace fs = std::filesystem;

SaveSystem::SaveSystem() {
    InitializeSaveDirectories();
}

void SaveSystem::InitializeSaveDirectories() {
    fs::create_directories(GetSavesDirectory(GameMode::PROCEDURAL_GENERATION));
    fs::create_directories(GetSavesDirectory(GameMode::PRELOADED_MAPS));

    for (int i = 1; i <= 5; i++) {
        fs::create_directories(GetSaveSlotPath(GameMode::PROCEDURAL_GENERATION, i));
        fs::create_directories(GetSaveSlotPath(GameMode::PRELOADED_MAPS, i));
    }
}

std::vector<SaveInfo> SaveSystem::GetAllSaves() {
    std::vector<SaveInfo> allSaves;

    auto proceduralSaves = GetSaves(GameMode::PROCEDURAL_GENERATION);
    for (auto& save : proceduralSaves) {
        if (!save.isEmpty) {
            save.gameMode = GameMode::PROCEDURAL_GENERATION;
            allSaves.push_back(save);
        }
    }

    auto preloadedSaves = GetSaves(GameMode::PRELOADED_MAPS);
    for (auto& save : preloadedSaves) {
        if (!save.isEmpty) {
            save.gameMode = GameMode::PRELOADED_MAPS;
            allSaves.push_back(save);
        }
    }

    std::sort(allSaves.begin(), allSaves.end(),
        [](const SaveInfo& a, const SaveInfo& b) {
            return a.slotNumber < b.slotNumber;
        });

    Logger::Log("GetAllSaves: Found " + std::to_string(allSaves.size()) + " saves total");
    return allSaves;
}

std::vector<SaveInfo> SaveSystem::GetSaves(GameMode mode) {
    std::vector<SaveInfo> saves;

    for (int slot = 1; slot <= 5; slot++) {
        SaveInfo info = GetSaveInfo(mode, slot);
        info.gameMode = mode;
        saves.push_back(info);
    }

    return saves;
}

SaveInfo SaveSystem::GetSaveInfo(GameMode mode, int slot) const {
    SaveInfo info;
    info.slotNumber = slot;
    info.savePath = GetSaveSlotPath(mode, slot);
    info.gameMode = mode;

    std::string infoFile = info.savePath + "/save_info.txt";

    if (fs::exists(infoFile)) {
        std::ifstream file(infoFile);
        if (file.is_open()) {
            std::getline(file, info.name);
            std::getline(file, info.creationDate);
            std::getline(file, info.lastPlayedDate);
            info.isEmpty = false;
            file.close();
        }
    }
    else {
        info.name = "Empty";
        info.creationDate = "";
        info.lastPlayedDate = "";
        info.isEmpty = true;
    }

    return info;
}

SaveInfo SaveSystem::GetSaveInfoUnified(int slot) const {
    SaveInfo proceduralInfo = GetSaveInfo(GameMode::PROCEDURAL_GENERATION, slot);
    if (!proceduralInfo.isEmpty) {
        return proceduralInfo;
    }

    SaveInfo preloadedInfo = GetSaveInfo(GameMode::PRELOADED_MAPS, slot);
    if (!preloadedInfo.isEmpty) {
        return preloadedInfo;
    }

    proceduralInfo.name = "Empty";
    return proceduralInfo;
}

bool SaveSystem::SaveExistsAnywhere(int slot) const {
    SaveInfo proceduralInfo = GetSaveInfo(GameMode::PROCEDURAL_GENERATION, slot);
    if (!proceduralInfo.isEmpty) {
        return true;
    }

    SaveInfo preloadedInfo = GetSaveInfo(GameMode::PRELOADED_MAPS, slot);
    return !preloadedInfo.isEmpty;
}

bool SaveSystem::CreateNewSave(GameMode mode, int slot, const std::string& name, const WorldConfig& config) {
    std::string savePath = GetSaveSlotPath(mode, slot);

    fs::create_directories(savePath);

    std::ofstream infoFile(savePath + "/save_info.txt");
    if (!infoFile.is_open()) {
        Logger::Log("ERROR: Cannot create save info file for slot " + std::to_string(slot));
        return false;
    }

    std::string currentTime = GetCurrentDateTime();
    infoFile << name << "\n";
    infoFile << currentTime << "\n";
    infoFile << currentTime << "\n";
    infoFile.close();

    // Сохраняем только world_gen.cfg, остальные файлы будут скопированы из шаблона
    if (!SaveWorldConfig(mode, slot, config)) {
        Logger::Log("ERROR: Failed to save world config for slot " + std::to_string(slot));
        return false;
    }

    // НЕ сохраняем player.cfg, tiles.json и другие - они будут скопированы из шаблона
    // при вызове CopyTemplateToSave()

    Logger::Log("Successfully created new save: " + name + " in slot " + std::to_string(slot) +
        " (mode: " + std::to_string(static_cast<int>(mode)) + ")");
    return true;
}

bool SaveSystem::SaveWorldConfig(GameMode mode, int slot, const WorldConfig& config) {
    std::string savePath = GetSaveSlotPath(mode, slot);
    std::string configPath = savePath + "/world_gen.cfg";

    std::stringstream content;
    content << "Width=" << config.GetWidth() << "\n";
    content << "Height=" << config.GetHeight() << "\n";
    content << "Seed=" << config.GetSeed() << "\n";
    content << "NoiseFrequency=" << config.GetNoiseFrequency() << "\n";
    content << "NeighborRadius=" << config.GetNeighborRadius() << "\n";
    content << "GenerationMode=" << (config.GetRandomGeneration() ? "1" : "2") << "\n";
    content << "WorldName=" << config.GetWorldName() << "\n";
    content << "PlayerStartX=" << config.GetPlayerStartX() << "\n";
    content << "PlayerStartY=" << config.GetPlayerStartY() << "\n";
    content << "PlayerMaxHP=" << config.GetPlayerMaxHP() << "\n";
    content << "PlayerMaxHunger=" << config.GetPlayerMaxHunger() << "\n";
    content << "EnableHP=" << (config.GetEnableHP() ? "true" : "false") << "\n";
    content << "EnableHunger=" << (config.GetEnableHunger() ? "true" : "false") << "\n";
    content << "EnableEnemies=" << (config.GetEnableEnemies() ? "true" : "false") << "\n";
    content << "EnemySpawnRate=" << config.GetEnemySpawnRate() << "\n";

    if (!config.GetSurvivalRules().empty()) {
        content << "SurvivalRules=" << config.GetSurvivalRules() << "\n";
    }
    if (!config.GetBirthRules().empty()) {
        content << "BirthRules=" << config.GetBirthRules() << "\n";
    }
    if (!config.GetDeathRules().empty()) {
        content << "DeathRules=" << config.GetDeathRules() << "\n";
    }

    return SaveConfigToFile(configPath, content.str());
}

bool SaveSystem::SavePlayerConfig(GameMode mode, int slot, const WorldConfig& config) {
    std::string savePath = GetSaveSlotPath(mode, slot);
    std::string configPath = savePath + "/player.cfg";

    std::stringstream content;
    content << "DefaultPlayerX=" << config.GetPlayerStartX() << "\n";
    content << "DefaultPlayerY=" << config.GetPlayerStartY() << "\n";
    content << "MAX_HP=" << config.GetPlayerMaxHP() << "\n";
    content << "MAX_HUNGER=" << config.GetPlayerMaxHunger() << "\n";
    content << "EnableHP=" << (config.GetEnableHP() ? "true" : "false") << "\n";
    content << "EnableHunger=" << (config.GetEnableHunger() ? "true" : "false") << "\n";
    content << "BaseXP=100\n";
    content << "XPMultiplier=1.5\n";
    content << "MoveCooldownMs=50\n";

    return SaveConfigToFile(configPath, content.str());
}

bool SaveSystem::SaveTilesConfig(GameMode mode, int slot, const WorldConfig& config) {
    std::string savePath = GetSaveSlotPath(mode, slot);
    std::string configPath = savePath + "/world_spawn.cfg";

    // Сохраняем правила спавна
    std::stringstream content;
    const auto& tileProbabilities = config.GetTileProbabilities();
    for (const auto& pair : tileProbabilities) {
        char tileChar = pair.first;
        const auto& probs = pair.second;

        if (probs.size() >= 3) {
            content << tileChar << "="
                << probs[0] << ":" << probs[1] << ":" << probs[2] << "\n";
        }
    }

    bool saved = SaveConfigToFile(configPath, content.str());

    // НЕ создаем tiles.json здесь - он должен быть скопирован из шаблона
    // или создан в WorldEditor при редактировании

    return saved;
}

bool SaveSystem::SaveAutomatonConfig(GameMode mode, int slot, const WorldConfig& config) {
    std::string savePath = GetSaveSlotPath(mode, slot);
    std::string configPath = savePath + "/cellular_automaton.cfg";

    std::stringstream content;
    if (!config.GetSurvivalRules().empty()) {
        content << ".\n";
        content << "survival=" << config.GetSurvivalRules() << "\n";
    }
    if (!config.GetBirthRules().empty()) {
        content << "birth=" << config.GetBirthRules() << "\n";
    }
    if (!config.GetDeathRules().empty()) {
        content << "death=" << config.GetDeathRules() << "\n";
    }

    return SaveConfigToFile(configPath, content.str());
}

WorldConfig SaveSystem::LoadWorldConfig(GameMode mode, int slot) {
    WorldConfig config;
    std::string savePath = GetSaveSlotPath(mode, slot);

    std::ifstream worldFile(savePath + "/world_gen.cfg");
    if (worldFile.is_open()) {
        std::string line;
        while (std::getline(worldFile, line)) {
            size_t delimiterPos = line.find('=');
            if (delimiterPos != std::string::npos) {
                std::string key = line.substr(0, delimiterPos);
                std::string value = line.substr(delimiterPos + 1);

                if (key == "Width") config.SetWidth(std::stoi(value));
                else if (key == "Height") config.SetHeight(std::stoi(value));
                else if (key == "Seed") config.SetSeed(std::stoi(value));
                else if (key == "NoiseFrequency") config.SetNoiseFrequency(std::stof(value));
                else if (key == "NeighborRadius") config.SetNeighborRadius(std::stoi(value));
                else if (key == "GenerationMode") config.SetRandomGeneration(value == "1");
                else if (key == "WorldName") config.SetWorldName(value);
                else if (key == "PlayerStartX") config.SetPlayerStartX(std::stoi(value));
                else if (key == "PlayerStartY") config.SetPlayerStartY(std::stoi(value));
                else if (key == "PlayerMaxHP") config.SetPlayerMaxHP(std::stoi(value));
                else if (key == "PlayerMaxHunger") config.SetPlayerMaxHunger(std::stoi(value));
                else if (key == "EnableHP") config.SetEnableHP(value == "true");
                else if (key == "EnableHunger") config.SetEnableHunger(value == "true");
                else if (key == "EnableEnemies") config.SetEnableEnemies(value == "true");
                else if (key == "EnemySpawnRate") config.SetEnemySpawnRate(std::stof(value));
                else if (key == "SurvivalRules") config.SetSurvivalRules(value);
                else if (key == "BirthRules") config.SetBirthRules(value);
                else if (key == "DeathRules") config.SetDeathRules(value);
            }
        }
        worldFile.close();
    }

    return config;
}

bool SaveSystem::SaveConfigToFile(const std::string& filePath, const std::string& content) {
    std::ofstream file(filePath);
    if (!file.is_open()) {
        Logger::Log("ERROR: Cannot open file for writing: " + filePath);
        return false;
    }

    file << content;
    file.close();
    return true;
}

bool SaveSystem::DeleteSave(GameMode mode, int slot) {
    std::string savePath = GetSaveSlotPath(mode, slot);

    try {
        if (fs::exists(savePath)) {
            uintmax_t removedCount = fs::remove_all(savePath);
            Logger::Log("Deleted save slot " + std::to_string(slot) +
                " in mode " + std::to_string(static_cast<int>(mode)) +
                ", removed " + std::to_string(removedCount) + " items");
            return true;
        }
        else {
            Logger::Log("WARNING: Save path does not exist: " + savePath);
        }
    }
    catch (const std::exception& e) {
        Logger::Log("ERROR deleting save: " + std::string(e.what()));
    }

    return false;
}

bool SaveSystem::LoadSave(GameMode mode, int slot) {
    SaveInfo info = GetSaveInfo(mode, slot);
    if (info.isEmpty) {
        Logger::Log("ERROR: Cannot load empty save slot " + std::to_string(slot));
        return false;
    }

    Logger::Log("Loading save from slot " + std::to_string(slot) +
        " (mode: " + std::to_string(static_cast<int>(mode)) + ")");

    m_loadedConfig = LoadWorldConfig(mode, slot);

    if (!LoadPlayerConfig(mode, slot)) {
        Logger::Log("WARNING: Failed to load player config, using defaults");
    }

    if (!LoadTilesConfig(mode, slot)) {
        Logger::Log("WARNING: Failed to load tiles config, using defaults");
    }

    if (!LoadAutomatonConfig(mode, slot)) {
        Logger::Log("WARNING: Failed to load automaton config, using defaults");
    }

    std::ofstream infoFile(info.savePath + "/save_info.txt");
    if (infoFile.is_open()) {
        infoFile << info.name << "\n";
        infoFile << info.creationDate << "\n";
        infoFile << GetCurrentDateTime() << "\n";
        infoFile.close();
    }

    Logger::Log("Successfully loaded save: " + info.name);
    return true;
}

bool SaveSystem::LoadSaveUnified(int slot) {
    SaveInfo proceduralInfo = GetSaveInfo(GameMode::PROCEDURAL_GENERATION, slot);
    if (!proceduralInfo.isEmpty) {
        return LoadSave(GameMode::PROCEDURAL_GENERATION, slot);
    }

    SaveInfo preloadedInfo = GetSaveInfo(GameMode::PRELOADED_MAPS, slot);
    if (!preloadedInfo.isEmpty) {
        return LoadSave(GameMode::PRELOADED_MAPS, slot);
    }

    Logger::Log("ERROR: Cannot load empty save slot " + std::to_string(slot));
    return false;
}

bool SaveSystem::LoadPlayerConfig(GameMode mode, int slot) {
    std::string savePath = GetSaveSlotPath(mode, slot);
    std::string configPath = savePath + "/player.cfg";

    return LoadConfigFromFile(configPath, [this](const std::string& key, const std::string& value) {
        if (key == "DefaultPlayerX") m_loadedConfig.SetPlayerStartX(std::stoi(value));
        else if (key == "DefaultPlayerY") m_loadedConfig.SetPlayerStartY(std::stoi(value));
        else if (key == "MAX_HP") m_loadedConfig.SetPlayerMaxHP(std::stoi(value));
        else if (key == "MAX_HUNGER") m_loadedConfig.SetPlayerMaxHunger(std::stoi(value));
        else if (key == "EnableHP") m_loadedConfig.SetEnableHP(value == "true");
        else if (key == "EnableHunger") m_loadedConfig.SetEnableHunger(value == "true");
        });
}

bool SaveSystem::LoadTilesConfig(GameMode mode, int slot) {
    std::string savePath = GetSaveSlotPath(mode, slot);
    std::string configPath = savePath + "/world_spawn.cfg";

    std::ifstream file(configPath);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    std::unordered_map<char, std::vector<float>> tileProbabilities;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        size_t delimiterPos = line.find('=');
        if (delimiterPos != std::string::npos) {
            char tileChar = line[0];
            std::string probabilitiesStr = line.substr(delimiterPos + 1);

            std::vector<float> probs;
            std::stringstream probStream(probabilitiesStr);
            std::string probToken;

            while (std::getline(probStream, probToken, ':')) {
                try {
                    probs.push_back(std::stof(probToken));
                }
                catch (const std::exception& e) {
                    probs.push_back(0.1f);
                }
            }
            while (probs.size() < 3) {
                probs.push_back(0.1f);
            }

            tileProbabilities[tileChar] = probs;
        }
    }

    file.close();
    m_loadedConfig.SetTileProbabilities(tileProbabilities);
    return true;
}

bool SaveSystem::LoadAutomatonConfig(GameMode mode, int slot) {
    std::string savePath = GetSaveSlotPath(mode, slot);
    std::string configPath = savePath + "/cellular_automaton.cfg";

    std::ifstream file(configPath);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    std::string currentTile;

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        if (line.length() == 1) {
            currentTile = line;
            continue;
        }

        size_t delimiterPos = line.find('=');
        if (delimiterPos != std::string::npos) {
            std::string key = line.substr(0, delimiterPos);
            std::string value = line.substr(delimiterPos + 1);

            if (key == "survival") m_loadedConfig.SetSurvivalRules(value);
            else if (key == "birth") m_loadedConfig.SetBirthRules(value);
            else if (key == "death") m_loadedConfig.SetDeathRules(value);
        }
    }

    file.close();
    return true;
}

bool SaveSystem::LoadConfigFromFile(const std::string& filePath, std::function<void(const std::string&, const std::string&)> parser) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;

        size_t delimiterPos = line.find('=');
        if (delimiterPos != std::string::npos) {
            std::string key = line.substr(0, delimiterPos);
            std::string value = line.substr(delimiterPos + 1);

            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);

            parser(key, value);
        }
    }

    file.close();
    return true;
}

bool SaveSystem::SaveGame(GameMode mode, int slot, const std::string& name) {
    std::string savePath = GetSaveSlotPath(mode, slot);
    std::ofstream infoFile(savePath + "/save_info.txt");

    if (infoFile.is_open()) {
        SaveInfo existingInfo = GetSaveInfo(mode, slot);
        std::string saveName = name.empty() ? existingInfo.name : name;

        infoFile << saveName << "\n";
        infoFile << (existingInfo.creationDate.empty() ? GetCurrentDateTime() : existingInfo.creationDate) << "\n";
        infoFile << GetCurrentDateTime() << "\n";
        infoFile.close();
        return true;
    }

    return false;
}

std::string SaveSystem::GetSavesDirectory(GameMode mode) const {
    std::string modeDir = (mode == GameMode::PROCEDURAL_GENERATION) ? PROCEDURAL_DIR : PRELOADED_DIR;
    return BASE_SAVES_DIR + "/" + modeDir;
}

std::string SaveSystem::GetSaveSlotPath(GameMode mode, int slot) const {
    return GetSavesDirectory(mode) + "/slot" + std::to_string(slot);
}

bool SaveSystem::IsSaveSlotEmpty(GameMode mode, int slot) const {
    return GetSaveInfo(mode, slot).isEmpty;
}

std::string SaveSystem::GetCurrentDateTime() const {
    time_t now = time(0);

    tm localTime;
    localtime_s(&localTime, &now);

    char buffer[80];
    strftime(buffer, sizeof(buffer), "%d.%m.%Y-%H:%M:%S", &localTime);
    return std::string(buffer);
}

bool SaveSystem::CopyDefaultConfigs(const std::string& sourceDir, const std::string& targetDir) {
    try {
        for (const auto& entry : fs::directory_iterator(sourceDir)) {
            if (entry.is_regular_file()) {
                fs::copy_file(entry.path(), targetDir + "/" + entry.path().filename().string(),
                    fs::copy_options::overwrite_existing);
            }
        }
        return true;
    }
    catch (const std::exception& e) {
        std::cout << "Error copying configs: " << e.what() << std::endl;
        return false;
    }
}

bool SaveSystem::CopyTemplateToSave(int templateSlot, int saveSlot, GameMode mode) {
    std::string templatePath = "templates/template" + std::to_string(templateSlot);
    std::string savePath = GetSaveSlotPath(mode, saveSlot);

    Logger::Log("=== COPYING TEMPLATE " + std::to_string(templateSlot) + " TO SAVE " + std::to_string(saveSlot) + " ===");
    Logger::Log("From: " + templatePath);
    Logger::Log("To: " + savePath);

    if (!fs::exists(templatePath)) {
        Logger::Log("ERROR: Template path doesn't exist: " + templatePath);
        return false;
    }

    // Создаем директорию сейва
    fs::create_directories(savePath);

    // Копируем ВСЕ файлы из шаблона
    bool allCopied = true;

    try {
        for (const auto& entry : fs::directory_iterator(templatePath)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                std::string sourceFile = templatePath + "/" + filename;
                std::string destFile = savePath + "/" + filename;

                // Не копируем template_info.txt - используем save_info.txt
                if (filename == "template_info.txt") {
                    // Читаем template_info.txt и создаем на его основе save_info.txt
                    std::ifstream templateInfoFile(sourceFile);
                    if (templateInfoFile.is_open()) {
                        std::string templateName, creationDate, modifiedDate;
                        std::getline(templateInfoFile, templateName);
                        std::getline(templateInfoFile, creationDate);
                        std::getline(templateInfoFile, modifiedDate);
                        templateInfoFile.close();

                        // Создаем save_info.txt с правильными данными
                        std::ofstream saveInfoFile(savePath + "/save_info.txt");
                        if (saveInfoFile.is_open()) {
                            saveInfoFile << templateName << " (from template)\n";
                            saveInfoFile << GetCurrentDateTime() << "\n";
                            saveInfoFile << GetCurrentDateTime() << "\n";
                            saveInfoFile.close();
                            Logger::Log("Created save_info.txt from template");
                        }
                    }
                }
                else {
                    fs::copy_file(sourceFile, destFile, fs::copy_options::overwrite_existing);
                    Logger::Log("Copied: " + filename);
                }
            }
        }

        // Проверяем, какие файлы были скопированы
        Logger::Log("=== COPIED FILES SUMMARY ===");
        for (const auto& entry : fs::directory_iterator(savePath)) {
            if (entry.is_regular_file()) {
                Logger::Log("  " + entry.path().filename().string());
            }
        }
        Logger::Log("=== END COPIED FILES SUMMARY ===");

    }
    catch (const std::exception& e) {
        Logger::Log("ERROR copying files: " + std::string(e.what()));
        allCopied = false;
    }

    return allCopied;
}

bool SaveSystem::CopyTemplateTilesToSave(int templateSlot, int saveSlot, GameMode mode) {
    std::string templatePath = "templates/template" + std::to_string(templateSlot);
    std::string savePath = GetSaveSlotPath(mode, saveSlot);

    std::string sourceFile = templatePath + "/tiles.json";
    std::string destFile = savePath + "/tiles.json";

    if (!fs::exists(sourceFile)) {
        Logger::Log("WARNING: Template tiles.json not found: " + sourceFile);
        return false;
    }

    try {
        fs::copy_file(sourceFile, destFile, fs::copy_options::overwrite_existing);
        Logger::Log("Copied tiles.json from template to save");
        return true;
    }
    catch (const std::exception& e) {
        Logger::Log("ERROR copying tiles.json: " + std::string(e.what()));
        return false;
    }
}