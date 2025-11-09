#include "SaveSystem.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <sstream>
#include "Logger.h"

namespace fs = std::filesystem;

SaveSystem::SaveSystem() {
    InitializeSaveDirectories();
}

void SaveSystem::InitializeSaveDirectories() {
    // Создаем структуру папок если их нет
    fs::create_directories(GetSavesDirectory(GameMode::PROCEDURAL_GENERATION));
    fs::create_directories(GetSavesDirectory(GameMode::PRELOADED_MAPS));

    // Создаем папки для слотов 1-5
    for (int i = 1; i <= 5; i++) {
        fs::create_directories(GetSaveSlotPath(GameMode::PROCEDURAL_GENERATION, i));
        fs::create_directories(GetSaveSlotPath(GameMode::PRELOADED_MAPS, i));
    }
}

std::vector<SaveInfo> SaveSystem::GetSaves(GameMode mode) {
    std::vector<SaveInfo> saves;

    for (int slot = 1; slot <= 5; slot++) {
        saves.push_back(GetSaveInfo(mode, slot));
    }

    return saves;
}

SaveInfo SaveSystem::GetSaveInfo(GameMode mode, int slot) const {
    SaveInfo info;
    info.slotNumber = slot;
    info.savePath = GetSaveSlotPath(mode, slot);

    // Проверяем есть ли файл с информацией о сейве
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

bool SaveSystem::CreateNewSave(GameMode mode, int slot, const std::string& name, const WorldEditorConfig& config) {
    std::string savePath = GetSaveSlotPath(mode, slot);

    // Создаем папку сейва если не существует
    fs::create_directories(savePath);

    // Создаем файл с информацией о сейве
    std::ofstream infoFile(savePath + "/save_info.txt");
    if (!infoFile.is_open()) {
        Logger::Log("ERROR: Cannot create save info file for slot " + std::to_string(slot));
        return false;
    }

    std::string currentTime = GetCurrentDateTime();
    infoFile << name << "\n";
    infoFile << currentTime << "\n"; // creation date
    infoFile << currentTime << "\n"; // last played date
    infoFile.close();

    // Сохраняем конфигурации из редактора
    if (!SaveWorldConfig(mode, slot, config)) {
        Logger::Log("ERROR: Failed to save world config for slot " + std::to_string(slot));
        return false;
    }

    if (!SavePlayerConfig(mode, slot, config)) {
        Logger::Log("ERROR: Failed to save player config for slot " + std::to_string(slot));
        return false;
    }

    if (!SaveTilesConfig(mode, slot, config)) {
        Logger::Log("ERROR: Failed to save tiles config for slot " + std::to_string(slot));
        return false;
    }

    if (!SaveAutomatonConfig(mode, slot, config)) {
        Logger::Log("ERROR: Failed to save automaton config for slot " + std::to_string(slot));
        return false;
    }

    Logger::Log("Successfully created new save: " + name + " in slot " + std::to_string(slot));
    return true;
}

bool SaveSystem::SaveWorldConfig(GameMode mode, int slot, const WorldEditorConfig& config) {
    std::string savePath = GetSaveSlotPath(mode, slot);
    std::string configPath = savePath + "/world_gen.cfg";

    std::stringstream content;
    content << "Width=" << config.width << "\n";
    content << "Height=" << config.height << "\n";
    content << "Seed=" << config.seed << "\n";
    content << "NoiseFrequency=" << config.noiseFrequency << "\n";
    content << "NeighborRadius=" << config.neighborRadius << "\n";
    content << "GenerationMode=" << (config.randomGeneration ? "1" : "2") << "\n";
    content << "WorldName=" << config.worldName << "\n";

    return SaveConfigToFile(configPath, content.str());
}

bool SaveSystem::SavePlayerConfig(GameMode mode, int slot, const WorldEditorConfig& config) {
    std::string savePath = GetSaveSlotPath(mode, slot);
    std::string configPath = savePath + "/player.cfg";

    std::stringstream content;
    content << "DefaultPlayerX=" << config.playerStartX << "\n";
    content << "DefaultPlayerY=" << config.playerStartY << "\n";
    content << "MAX_HP=" << config.playerMaxHP << "\n";
    content << "MAX_HUNGER=" << config.playerMaxHunger << "\n";
    content << "EnableHP=" << (config.enableHP ? "true" : "false") << "\n";
    content << "EnableHunger=" << (config.enableHunger ? "true" : "false") << "\n";
    content << "BaseXP=100\n";
    content << "XPMultiplier=1.5\n";
    content << "MoveCooldownMs=50\n";

    return SaveConfigToFile(configPath, content.str());
}

bool SaveSystem::SaveTilesConfig(GameMode mode, int slot, const WorldEditorConfig& config) {
    std::string savePath = GetSaveSlotPath(mode, slot);
    std::string configPath = savePath + "/world_spawn.cfg";

    std::stringstream content;
    for (const auto& pair : config.tileProbabilities) {
        char tileChar = pair.first;
        const auto& probs = pair.second;

        if (probs.size() >= 3) {
            content << tileChar << "="
                << probs[0] << ":" << probs[1] << ":" << probs[2] << "\n";
        }
    }

    return SaveConfigToFile(configPath, content.str());
}

bool SaveSystem::SaveAutomatonConfig(GameMode mode, int slot, const WorldEditorConfig& config) {
    std::string savePath = GetSaveSlotPath(mode, slot);
    std::string configPath = savePath + "/cellular_automaton.cfg";

    std::stringstream content;
    // Сохраняем правила клеточного автомата
    if (!config.survivalRules.empty()) {
        content << ".\n";  // Тайл, к которому применяются правила
        content << "survival=" << config.survivalRules << "\n";
    }
    if (!config.birthRules.empty()) {
        content << "birth=" << config.birthRules << "\n";
    }
    if (!config.deathRules.empty()) {
        content << "death=" << config.deathRules << "\n";
    }

    return SaveConfigToFile(configPath, content.str());
}

WorldEditorConfig SaveSystem::LoadWorldConfig(GameMode mode, int slot) {
    WorldEditorConfig config;
    std::string savePath = GetSaveSlotPath(mode, slot);

    // Загружаем конфиг мира
    std::ifstream worldFile(savePath + "/world_gen.cfg");
    if (worldFile.is_open()) {
        std::string line;
        while (std::getline(worldFile, line)) {
            size_t delimiterPos = line.find('=');
            if (delimiterPos != std::string::npos) {
                std::string key = line.substr(0, delimiterPos);
                std::string value = line.substr(delimiterPos + 1);

                if (key == "Width") config.width = std::stoi(value);
                else if (key == "Height") config.height = std::stoi(value);
                else if (key == "Seed") config.seed = std::stoi(value);
                else if (key == "NoiseFrequency") config.noiseFrequency = std::stof(value);
                else if (key == "NeighborRadius") config.neighborRadius = std::stoi(value);
                else if (key == "GenerationMode") config.randomGeneration = (value == "1");
                else if (key == "WorldName") config.worldName = value;
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
        // Удаляем всю папку сейва
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

    Logger::Log("Loading save from slot " + std::to_string(slot));

    // Загружаем все конфигурации
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

    // Обновляем дату последнего запуска
    std::ofstream infoFile(info.savePath + "/save_info.txt");
    if (infoFile.is_open()) {
        infoFile << info.name << "\n";
        infoFile << info.creationDate << "\n";
        infoFile << GetCurrentDateTime() << "\n"; // Обновляем last played
        infoFile.close();
    }

    Logger::Log("Successfully loaded save: " + info.name);
    return true;
}

bool SaveSystem::LoadPlayerConfig(GameMode mode, int slot) {
    std::string savePath = GetSaveSlotPath(mode, slot);
    std::string configPath = savePath + "/player.cfg";

    return LoadConfigFromFile(configPath, [this](const std::string& key, const std::string& value) {
        if (key == "DefaultPlayerX") m_loadedConfig.playerStartX = std::stoi(value);
        else if (key == "DefaultPlayerY") m_loadedConfig.playerStartY = std::stoi(value);
        else if (key == "MAX_HP") m_loadedConfig.playerMaxHP = std::stoi(value);
        else if (key == "MAX_HUNGER") m_loadedConfig.playerMaxHunger = std::stoi(value);
        else if (key == "EnableHP") m_loadedConfig.enableHP = (value == "true");
        else if (key == "EnableHunger") m_loadedConfig.enableHunger = (value == "true");
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
                    probs.push_back(0.1f); // значение по умолчанию при ошибке
                }
            }

            // Дополняем до 3 значений если нужно
            while (probs.size() < 3) {
                probs.push_back(0.1f);
            }

            m_loadedConfig.tileProbabilities[tileChar] = probs;
        }
    }

    file.close();
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

        // Если строка состоит из одного символа - это тайл
        if (line.length() == 1) {
            currentTile = line;
            continue;
        }

        size_t delimiterPos = line.find('=');
        if (delimiterPos != std::string::npos) {
            std::string key = line.substr(0, delimiterPos);
            std::string value = line.substr(delimiterPos + 1);

            if (key == "survival") m_loadedConfig.survivalRules = value;
            else if (key == "birth") m_loadedConfig.birthRules = value;
            else if (key == "death") m_loadedConfig.deathRules = value;
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

            // Убираем пробелы
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
    // TODO: Реализовать сохранение текущего состояния игры
    // Пока просто обновляем информацию о сейве
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

    // Безопасная версия localtime
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