#pragma once
#include <string>

struct WorldGenConfig {
    int width;
    int height;
    int seed;
    bool useRandomSeed;
    float noiseFrequency;

    WorldGenConfig();
    bool LoadFromFile(const std::string& filePath);
};