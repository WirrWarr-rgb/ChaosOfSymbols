#pragma once
#include <string>
#include <map>

struct BiomeRule {
    float minHeight;
    float maxHeight;
    int tileId;
    float probability;
};

//test
struct WorldGenConfig {
    int width;
    int height;
    int seed;
    bool useRandomSeed;
    float noiseFrequency;
    std::map<std::string, BiomeRule> biomeRules;
    std::map<int, float> specialFeatures;

    WorldGenConfig();
    bool LoadFromFile(const std::string& filePath);
    bool SaveToFile(const std::string& filePath);
};