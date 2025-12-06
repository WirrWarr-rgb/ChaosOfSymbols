#include "TemplateSystem.h"
#include "Logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <functional>

namespace fs = std::filesystem;

const std::string TemplateSystem::BASE_TEMPLATES_DIR = "templates";
const std::string TemplateSystem::TEMPLATE_INFO_FILE = "template_info.txt";

TemplateSystem::TemplateSystem() {
}

bool TemplateSystem::Initialize() {
    try {
        fs::create_directories(GetTemplatesDirectory());

        for (int i = 1; i <= MAX_TEMPLATES; i++) {
            std::string slotPath = GetTemplateSlotPath(i);
            fs::create_directories(slotPath);
        }

        Logger::Log("TemplateSystem initialized successfully");
        return true;
    }
    catch (const std::exception& e) {
        Logger::Log("ERROR initializing TemplateSystem: " + std::string(e.what()));
        return false;
    }
}

TemplateInfo TemplateSystem::GetTemplateInfo(int slot) const {
    TemplateInfo info;
    info.slotNumber = slot;
    info.templatePath = GetTemplateSlotPath(slot);
    info.isEmpty = true;

    std::string infoFile = info.templatePath + "/" + TEMPLATE_INFO_FILE;

    if (fs::exists(infoFile)) {
        std::ifstream file(infoFile);
        if (file.is_open()) {
            std::getline(file, info.name);
            std::getline(file, info.creationDate);
            std::getline(file, info.lastModifiedDate);

            if (info.name != "Empty" && !info.name.empty()) {
                info.isEmpty = false;
            }

            file.close();
        }
    }
    else {
        info.name = "Empty";
        info.creationDate = "";
        info.lastModifiedDate = "";
    }

    return info;
}


bool TemplateSystem::CreateTemplate(int slot, const std::string& name, const WorldEditorConfig& config) {
    if (slot < 1 || slot > MAX_TEMPLATES) {
        Logger::Log("ERROR: Invalid template slot: " + std::to_string(slot));
        return false;
    }

    std::string templatePath = GetTemplateSlotPath(slot);

    try {
        fs::create_directories(templatePath);

        std::ofstream infoFile(templatePath + "/" + TEMPLATE_INFO_FILE);
        if (!infoFile.is_open()) {
            Logger::Log("ERROR: Cannot create template info file for slot " + std::to_string(slot));
            return false;
        }

        std::string currentTime = GetCurrentDateTime();
        infoFile << name << "\n";
        infoFile << currentTime << "\n";
        infoFile << currentTime << "\n";
        infoFile.close();

        if (!SaveWorldConfig(slot, config)) {
            Logger::Log("ERROR: Failed to save world config for template slot " + std::to_string(slot));
            return false;
        }

        if (!SavePlayerConfig(slot, config)) {
            Logger::Log("ERROR: Failed to save player config for template slot " + std::to_string(slot));
            return false;
        }

        if (!SaveTilesConfig(slot, config)) {
            Logger::Log("ERROR: Failed to save tiles config for template slot " + std::to_string(slot));
            return false;
        }

        if (!SaveAutomatonConfig(slot, config)) {
            Logger::Log("ERROR: Failed to save automaton config for template slot " + std::to_string(slot));
            return false;
        }

        if (!SaveFoodConfig(slot, config)) {
            Logger::Log("ERROR: Failed to save food config for template slot " + std::to_string(slot));
            return false;
        }

        Logger::Log("Successfully created template: " + name + " in slot " + std::to_string(slot));
        return true;
    }
    catch (const std::exception& e) {
        Logger::Log("ERROR creating template: " + std::string(e.what()));
        return false;
    }
}

bool TemplateSystem::LoadTemplate(int slot, WorldEditorConfig& config) {
    if (slot < 1 || slot > MAX_TEMPLATES) {
        Logger::Log("ERROR: Invalid template slot: " + std::to_string(slot));
        return false;
    }

    std::string templatePath = GetTemplateSlotPath(slot);

    TemplateInfo info = GetTemplateInfo(slot);
    if (info.isEmpty) {
        Logger::Log("ERROR: Cannot load empty template slot " + std::to_string(slot));
        return false;
    }

    try {
        Logger::Log("Loading template from slot " + std::to_string(slot));

        if (!LoadWorldConfig(slot, config)) {
            Logger::Log("WARNING: Failed to load world config from template, using defaults");
        }

        if (!LoadPlayerConfig(slot, config)) {
            Logger::Log("WARNING: Failed to load player config from template, using defaults");
        }

        if (!LoadTilesConfig(slot, config)) {
            Logger::Log("WARNING: Failed to load tiles config from template, using defaults");
        }

        if (!LoadAutomatonConfig(slot, config)) {
            Logger::Log("WARNING: Failed to load automaton config from template, using defaults");
        }

        if (!LoadFoodConfig(slot, config)) {
            Logger::Log("WARNING: Failed to load food config from template, using defaults");
        }

        std::ofstream infoFile(templatePath + "/" + TEMPLATE_INFO_FILE);
        if (infoFile.is_open()) {
            infoFile << info.name << "\n";
            infoFile << info.creationDate << "\n";
            infoFile << GetCurrentDateTime() << "\n";
            infoFile.close();
        }

        Logger::Log("Successfully loaded template: " + info.name);
        return true;
    }
    catch (const std::exception& e) {
        Logger::Log("ERROR loading template: " + std::string(e.what()));
        return false;
    }
}

bool TemplateSystem::DeleteTemplate(int slot) {
    if (slot < 1 || slot > MAX_TEMPLATES) {
        Logger::Log("ERROR: Invalid template slot: " + std::to_string(slot));
        return false;
    }

    std::string templatePath = GetTemplateSlotPath(slot);
    std::string infoFile = templatePath + "/" + TEMPLATE_INFO_FILE;

    try {
        if (fs::exists(infoFile)) {
            fs::remove(infoFile);

            std::vector<std::string> configFiles = {
                templatePath + "/world_gen.cfg",
                templatePath + "/player.cfg",
                templatePath + "/tiles.json",
                templatePath + "/cellular_automaton.cfg",
                templatePath + "/food.cfg"
            };

            for (const auto& file : configFiles) {
                if (fs::exists(file)) {
                    fs::remove(file);
                }
            }

            Logger::Log("Deleted template slot " + std::to_string(slot));
            return true;
        }
        else {
            Logger::Log("WARNING: Template info file does not exist for slot " + std::to_string(slot));
            return false;
        }
    }
    catch (const std::exception& e) {
        Logger::Log("ERROR deleting template: " + std::string(e.what()));
        return false;
    }
}

bool TemplateSystem::SaveTemplate(int slot, const WorldEditorConfig& config) {
    return CreateTemplate(slot, config.worldName, config);
}

bool TemplateSystem::IsTemplateSlotEmpty(int slot) const {
    return GetTemplateInfo(slot).isEmpty;
}

std::string TemplateSystem::GetTemplatesDirectory() const {
    return BASE_TEMPLATES_DIR;
}

std::string TemplateSystem::GetTemplateSlotPath(int slot) const {
    return GetTemplatesDirectory() + "/template" + std::to_string(slot);
}

bool TemplateSystem::SaveWorldConfig(int slot, const WorldEditorConfig& config) {
    std::string templatePath = GetTemplateSlotPath(slot);
    std::string configPath = templatePath + "/world_gen.cfg";

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

bool TemplateSystem::SavePlayerConfig(int slot, const WorldEditorConfig& config) {
    std::string templatePath = GetTemplateSlotPath(slot);
    std::string configPath = templatePath + "/player.cfg";

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

bool TemplateSystem::SaveTilesConfig(int slot, const WorldEditorConfig& config) {
    std::string templatePath = GetTemplateSlotPath(slot);
    std::string configPath = templatePath + "/tiles.json";

    std::stringstream content;
    content << "[\n";

    content << "  {\n";
    content << "    \"id\": 0,\n";
    content << "    \"name\": \"air\",\n";
    content << "    \"character\": \" \",\n";
    content << "    \"color\": 0,\n";
    content << "    \"isPassable\": true,\n";
    content << "    \"isDestructible\": false,\n";
    content << "    \"damage\": 0\n";
    content << "  }\n";
    content << "]";

    return SaveConfigToFile(configPath, content.str());
}

bool TemplateSystem::SaveAutomatonConfig(int slot, const WorldEditorConfig& config) {
    std::string templatePath = GetTemplateSlotPath(slot);
    std::string configPath = templatePath + "/cellular_automaton.cfg";

    std::stringstream content;
    if (!config.survivalRules.empty()) {
        content << ".\n";
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

bool TemplateSystem::SaveFoodConfig(int slot, const WorldEditorConfig& config) {
    std::string templatePath = GetTemplateSlotPath(slot);
    std::string configPath = templatePath + "/food.cfg";

    std::stringstream content;
    content << "# ID Name Symbol Color HungerRestore HpRestore SpawnWeight Experience\n";
    content << "1   Apple   a   2      10           5         30          5\n";
    content << "2   Bread   b   6      20           0         20          10\n";
    content << "3   Meat    m   4      30           20        10          20\n";

    return SaveConfigToFile(configPath, content.str());
}

bool TemplateSystem::LoadWorldConfig(int slot, WorldEditorConfig& config) {
    std::string templatePath = GetTemplateSlotPath(slot);
    std::string configPath = templatePath + "/world_gen.cfg";

    if (!fs::exists(configPath)) {
        return false;
    }

    return LoadConfigFromFile(configPath, [&config](const std::string& key, const std::string& value) {
        if (key == "Width") config.width = std::stoi(value);
        else if (key == "Height") config.height = std::stoi(value);
        else if (key == "Seed") config.seed = std::stoi(value);
        else if (key == "NoiseFrequency") config.noiseFrequency = std::stof(value);
        else if (key == "NeighborRadius") config.neighborRadius = std::stoi(value);
        else if (key == "GenerationMode") config.randomGeneration = (value == "1");
        else if (key == "WorldName") config.worldName = value;
        });
}

bool TemplateSystem::LoadPlayerConfig(int slot, WorldEditorConfig& config) {
    std::string templatePath = GetTemplateSlotPath(slot);
    std::string configPath = templatePath + "/player.cfg";

    if (!fs::exists(configPath)) {
        return false;
    }

    return LoadConfigFromFile(configPath, [&config](const std::string& key, const std::string& value) {
        if (key == "DefaultPlayerX") config.playerStartX = std::stoi(value);
        else if (key == "DefaultPlayerY") config.playerStartY = std::stoi(value);
        else if (key == "MAX_HP") config.playerMaxHP = std::stoi(value);
        else if (key == "MAX_HUNGER") config.playerMaxHunger = std::stoi(value);
        else if (key == "EnableHP") config.enableHP = (value == "true");
        else if (key == "EnableHunger") config.enableHunger = (value == "true");
        });
}

bool TemplateSystem::LoadTilesConfig(int slot, WorldEditorConfig& config) {
    std::string templatePath = GetTemplateSlotPath(slot);
    std::string configPath = templatePath + "/world_spawn.cfg";

    if (!fs::exists(configPath)) {
        return false;
    }

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
                    probs.push_back(0.1f);
                }
            }
            while (probs.size() < 3) {
                probs.push_back(0.1f);
            }

            config.tileProbabilities[tileChar] = probs;
        }
    }

    file.close();
    return true;
}

bool TemplateSystem::LoadAutomatonConfig(int slot, WorldEditorConfig& config) {
    std::string templatePath = GetTemplateSlotPath(slot);
    std::string configPath = templatePath + "/cellular_automaton.cfg";

    if (!fs::exists(configPath)) {
        return false;
    }

    std::ifstream file(configPath);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;

        size_t delimiterPos = line.find('=');
        if (delimiterPos != std::string::npos) {
            std::string key = line.substr(0, delimiterPos);
            std::string value = line.substr(delimiterPos + 1);

            if (key == "survival") config.survivalRules = value;
            else if (key == "birth") config.birthRules = value;
            else if (key == "death") config.deathRules = value;
        }
    }

    file.close();
    return true;
}

bool TemplateSystem::LoadFoodConfig(int slot, WorldEditorConfig& config) {
    return true;
}

bool TemplateSystem::SaveConfigToFile(const std::string& filePath, const std::string& content) {
    std::ofstream file(filePath);
    if (!file.is_open()) {
        Logger::Log("ERROR: Cannot open file for writing: " + filePath);
        return false;
    }

    file << content;
    file.close();
    return true;
}

bool TemplateSystem::LoadConfigFromFile(const std::string& filePath,
    std::function<void(const std::string&, const std::string&)> parser) {
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

std::string TemplateSystem::GetCurrentDateTime() const {
    time_t now = time(0);
    tm localTime;
    localtime_s(&localTime, &now);

    char buffer[80];
    strftime(buffer, sizeof(buffer), "%d.%m.%Y-%H:%M:%S", &localTime);
    return std::string(buffer);
}

std::vector<TemplateInfo> TemplateSystem::GetTemplates() {
    std::vector<TemplateInfo> templates;

    for (int slot = 1; slot <= MAX_TEMPLATES; slot++) {
        TemplateInfo info = GetTemplateInfo(slot);
        templates.push_back(info);
    }

    return templates;
}