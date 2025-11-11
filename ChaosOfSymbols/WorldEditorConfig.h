#pragma once
#include <string>
#include <unordered_map>
#include <vector>

struct WorldEditorConfig {
    // World tab
    std::string worldName = "New_World";
    int width = 80;
    int height = 40;
    bool randomGeneration = true;
    int seed = 1337;
    float noiseFrequency = 0.7f;
    int neighborRadius = 3;

    // Player tab
    int playerStartX = 40;
    int playerStartY = 20;
    int playerMaxHP = 30;
    int playerMaxHunger = 20;
    bool enableHP = true;
    bool enableHunger = true;

    // Tiles tab
    std::unordered_map<char, std::vector<float>> tileProbabilities;

    // Cellular Automaton tab
    std::string survivalRules = "";
    std::string birthRules = "";
    std::string deathRules = "";

    // Enemies tab
    bool enableEnemies = false;
    float enemySpawnRate = 0.1f;

    // Food tab (можно добавить позже)
    // Win/Lose conditions (можно добавить позже)
};