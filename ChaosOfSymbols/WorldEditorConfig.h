#pragma once
#include <string>
#include <unordered_map>
#include <vector>

struct WorldEditorConfig {
    std::string worldName = "New_World";
    int width = 80;
    int height = 40;
    bool randomGeneration = true;
    int seed = 1337;
    float noiseFrequency = 0.7f;
    int neighborRadius = 3;

    int playerStartX = 40;
    int playerStartY = 20;
    int playerMaxHP = 30;
    int playerMaxHunger = 20;
    bool enableHP = true;
    bool enableHunger = true;

    std::unordered_map<char, std::vector<float>> tileProbabilities;

    std::string survivalRules = "";
    std::string birthRules = "";
    std::string deathRules = "";

    bool enableEnemies = false;
    float enemySpawnRate = 0.1f;
};