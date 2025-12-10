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
    fs::create_directories(BASE_SAVES_DIR);

    for (int i = 1; i <= 10; i++) {
        fs::create_directories(BASE_SAVES_DIR + "/slot" + std::to_string(i));
    }
}

std::vector<SaveInfo> SaveSystem::GetAllSaves() {
    std::vector<SaveInfo> allSaves;

    for (int slot = 1; slot <= 10; slot++) {
        SaveInfo info = GetSaveInfo(slot);
        info.gameMode = GameMode::PROCEDURAL_GENERATION;
        allSaves.push_back(info);
    }

    Logger::Log("GetAllSaves: Found " + std::to_string(allSaves.size()) + " saves total");
    return allSaves;
}

SaveInfo SaveSystem::GetSaveInfo(GameMode mode, int slot) const {
    return GetSaveInfo(slot);
}

SaveInfo SaveSystem::GetSaveInfo(int slot) const {
    SaveInfo info;
    info.slotNumber = slot;
    info.savePath = BASE_SAVES_DIR + "/slot" + std::to_string(slot);
    info.gameMode = GameMode::PROCEDURAL_GENERATION;

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
        info.name = "";
        info.creationDate = "";
        info.lastPlayedDate = "";
        info.isEmpty = true;
    }

    return info;
}

SaveInfo SaveSystem::GetSaveInfoUnified(int slot) const {
    return GetSaveInfo(slot);
}

bool SaveSystem::SaveExistsAnywhere(int slot) const {
    SaveInfo info = GetSaveInfo(slot);
    return !info.isEmpty;
}

bool SaveSystem::CreateNewSave(int slot, const std::string& name, const WorldConfig& config) {
    std::string savePath = BASE_SAVES_DIR + "/slot" + std::to_string(slot);

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

    if (!SaveWorldConfig(slot, config)) {
        Logger::Log("ERROR: Failed to save world config for slot " + std::to_string(slot));
        return false;
    }

    Logger::Log("Successfully created new save: " + name + " in slot " + std::to_string(slot));
    return true;
}

bool SaveSystem::SaveWorldConfig(int slot, const WorldConfig& config) {
    std::string savePath = BASE_SAVES_DIR + "/slot" + std::to_string(slot);
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

bool SaveSystem::SavePlayerConfig(int slot, const WorldConfig& config) {
    std::string savePath = GetSaveSlotPath(slot);
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

bool SaveSystem::SaveTilesConfig(int slot, const WorldConfig& config) {
    std::string savePath = GetSaveSlotPath(slot);
    std::string configPath = savePath + "/world_spawn.cfg";

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

    return SaveConfigToFile(configPath, content.str());
}

bool SaveSystem::SaveAutomatonConfig(int slot, const WorldConfig& config) {
    std::string savePath = GetSaveSlotPath(slot);
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

WorldConfig SaveSystem::LoadWorldConfig(int slot) {
    WorldConfig config;
    std::string savePath = GetSaveSlotPath(slot);

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

bool SaveSystem::DeleteSave(int slot) {
    std::string savePath = GetSaveSlotPath(slot);

    try {
        if (fs::exists(savePath)) {
            uintmax_t removedCount = fs::remove_all(savePath);
            Logger::Log("Deleted save slot " + std::to_string(slot) +
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

bool SaveSystem::LoadSave(int slot) {
    SaveInfo info = GetSaveInfo(slot);
    if (info.isEmpty) {
        Logger::Log("ERROR: Cannot load empty save slot " + std::to_string(slot));
        return false;
    }

    Logger::Log("Loading save from slot " + std::to_string(slot));

    m_loadedConfig = LoadWorldConfig(slot);

    if (!LoadPlayerConfig(slot)) {
        Logger::Log("WARNING: Failed to load player config, using defaults");
    }

    if (!LoadTilesConfig(slot)) {
        Logger::Log("WARNING: Failed to load tiles config, using defaults");
    }

    if (!LoadAutomatonConfig(slot)) {
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
    SaveInfo info = GetSaveInfo(slot);
    if (!info.isEmpty) {
        return LoadSave(slot);
    }

    Logger::Log("ERROR: Cannot load empty save slot " + std::to_string(slot));
    return false;
}

bool SaveSystem::LoadPlayerConfig(int slot) {
    std::string savePath = GetSaveSlotPath(slot);
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

bool SaveSystem::LoadTilesConfig(int slot) {
    std::string savePath = GetSaveSlotPath(slot);
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

bool SaveSystem::LoadAutomatonConfig(int slot) {
    std::string savePath = GetSaveSlotPath(slot);
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

bool SaveSystem::SaveGame(int slot, const std::string& name) {
    std::string savePath = GetSaveSlotPath(slot);
    std::ofstream infoFile(savePath + "/save_info.txt");

    if (infoFile.is_open()) {
        SaveInfo existingInfo = GetSaveInfo(slot);
        std::string saveName = name.empty() ? existingInfo.name : name;

        infoFile << saveName << "\n";
        infoFile << (existingInfo.creationDate.empty() ? GetCurrentDateTime() : existingInfo.creationDate) << "\n";
        infoFile << GetCurrentDateTime() << "\n";
        infoFile.close();
        return true;
    }

    return false;
}

std::string SaveSystem::GetSaveSlotPath(int slot) const {
    return BASE_SAVES_DIR + "/slot" + std::to_string(slot);
}

bool SaveSystem::IsSaveSlotEmpty(int slot) const {
    return GetSaveInfo(slot).isEmpty;
}

std::string SaveSystem::GetCurrentDateTime() const {
    time_t now = time(0);

    tm localTime;
    localtime_s(&localTime, &now);

    char buffer[80];
    strftime(buffer, sizeof(buffer), "%d.%m.%Y-%H:%M:%S", &localTime);
    return std::string(buffer);
}

bool SaveSystem::CopyTemplateToSave(int templateSlot, int saveSlot) {
    std::string templatePath = "templates/template" + std::to_string(templateSlot);
    std::string savePath = BASE_SAVES_DIR + "/slot" + std::to_string(saveSlot);

    Logger::Log("=== COPYING TEMPLATE " + std::to_string(templateSlot) + " TO SAVE " + std::to_string(saveSlot) + " ===");
    Logger::Log("From: " + templatePath);
    Logger::Log("To: " + savePath);

    if (!fs::exists(templatePath)) {
        Logger::Log("ERROR: Template path doesn't exist: " + templatePath);
        return false;
    }

    fs::create_directories(savePath);

    bool allCopied = true;

    try {
        for (const auto& entry : fs::directory_iterator(templatePath)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                std::string sourceFile = templatePath + "/" + filename;
                std::string destFile = savePath + "/" + filename;

                if (filename == "template_info.txt") {
                    std::ifstream templateInfoFile(sourceFile);
                    if (templateInfoFile.is_open()) {
                        std::string templateName, creationDate, modifiedDate;
                        std::getline(templateInfoFile, templateName);
                        std::getline(templateInfoFile, creationDate);
                        std::getline(templateInfoFile, modifiedDate);
                        templateInfoFile.close();

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