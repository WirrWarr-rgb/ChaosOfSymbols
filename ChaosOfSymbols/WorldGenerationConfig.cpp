#include "WorldGenerationConfig.h"
#include "Logger.h"
#include <fstream>
#include <iostream>
#include <sstream>

WorldGenConfig::WorldGenConfig()
    : width(100), height(50), seed(1337), useRandomSeed(true), noiseFrequency(0.05f) {}

bool WorldGenConfig::LoadFromFile(const std::string& filePath) {
    Logger::Log("Loading world config from: " + filePath);

    std::ifstream file(filePath);
    if (!file.is_open()) {
        Logger::Log("ERROR: World generation config not found: " + filePath);
        return false;
    }

    std::string line;
    std::string currentSection;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        if (line[0] == '[' && line.back() == ']') {
            currentSection = line.substr(1, line.length() - 2);
            Logger::Log("Found section: " + currentSection);
            continue;
        }

        size_t delimiterPos = line.find('=');
        if (delimiterPos == std::string::npos) continue;

        std::string key = line.substr(0, delimiterPos);
        std::string value = line.substr(delimiterPos + 1);

        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);

        Logger::Log("Parsing " + key + " = " + value + " in section " + currentSection);

        if (currentSection == "World") {
            if (key == "Width") {
                width = std::stoi(value);
                Logger::Log("Set width to " + std::to_string(width));
            }
            else if (key == "Height") {
                height = std::stoi(value);
                Logger::Log("Set height to " + std::to_string(height));
            }
            else if (key == "Seed") {
                seed = std::stoi(value);
                Logger::Log("Set seed to " + std::to_string(seed));
            }
            else if (key == "UseRandomSeed") {
                useRandomSeed = (value == "true");
                Logger::Log("Set useRandomSeed to " + std::string(useRandomSeed ? "true" : "false"));
            }
            else if (key == "NoiseFrequency") {
                noiseFrequency = std::stof(value);
                Logger::Log("Set noiseFrequency to " + std::to_string(noiseFrequency));
            }
        }
        else if (currentSection == "Biomes") {
            std::stringstream ss(value);
            std::string token;
            BiomeRule rule;

            if (std::getline(ss, token, ',')) {
                rule.minHeight = std::stof(token);
            }
            if (std::getline(ss, token, ',')) {
                rule.maxHeight = std::stof(token);
            }
            if (std::getline(ss, token, ',')) {
                rule.tileId = std::stoi(token);
            }
            if (std::getline(ss, token, ',')) {
                rule.probability = std::stof(token);
            }

            biomeRules[key] = rule;
            Logger::Log("Added biome: " + key + " [" + std::to_string(rule.minHeight) +
                " to " + std::to_string(rule.maxHeight) + "] -> tile " +
                std::to_string(rule.tileId) + " prob " + std::to_string(rule.probability));
        }
        else if (currentSection == "SpecialFeatures") {
            int tileId = std::stoi(key);
            float probability = std::stof(value);
            specialFeatures[tileId] = probability;
            Logger::Log("Special feature: tile " + std::to_string(tileId) +
                " probability " + std::to_string(probability));
        }
    }

    file.close();
    Logger::Log("Config loaded successfully. Biomes: " + std::to_string(biomeRules.size()) +
        ", Special features: " + std::to_string(specialFeatures.size()));
    return true;
}

bool WorldGenConfig::SaveToFile(const std::string& filePath) {
    std::ofstream file(filePath);
    if (!file.is_open()) {
        Logger::Log("ERROR: Failed to save world generation config: " + filePath);
        return false;
    }

    file << "# World Generation Configuration\n\n";

    file << "[World]\n";
    file << "Width=" << width << "\n";
    file << "Height=" << height << "\n";
    file << "Seed=" << seed << "\n";
    file << "UseRandomSeed=" << (useRandomSeed ? "true" : "false") << "\n";
    file << "NoiseFrequency=" << noiseFrequency << "\n\n";

    file << "[Biomes]\n";
    for (const auto& biome : biomeRules) {
        file << biome.first << "=" << biome.second.minHeight << ","
            << biome.second.maxHeight << "," << biome.second.tileId << ","
            << biome.second.probability << "\n";
    }
    file << "\n";

    file << "[SpecialFeatures]\n";
    for (const auto& feature : specialFeatures) {
        file << feature.first << "=" << feature.second << "\n";
    }

    file.close();
    Logger::Log("World generation config saved: " + filePath);
    return true;
}