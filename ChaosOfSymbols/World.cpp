#include "World.h"
#include "Logger.h"
#include <iostream>
#include <fstream>
#include <random>

World::World()
    : m_width(0), m_height(0), m_currentSeed(0) {
    Logger::Log("World constructor: creating empty world");
}

void World::Generate(int seed) {
    m_currentSeed = seed;
    m_noiseGenerator.SetSeed(seed);
    m_noiseGenerator.SetFrequency(0.05f);

    if (m_width == 0 || m_height == 0) {
        m_width = 100;
        m_height = 50;
        m_map.resize(m_height, std::vector<int>(m_width, 0));
        Logger::Log("WARNING: Using default size in Generate: " +
            std::to_string(m_width) + "x" + std::to_string(m_height));
    }

    Logger::Log("Generating world with seed: " + std::to_string(seed) +
        ", size: " + std::to_string(m_width) + "x" + std::to_string(m_height));

    for (int y = 0; y < m_height; y++) {
        for (int x = 0; x < m_width; x++) {
            float noiseValue = m_noiseGenerator.GetNoise(static_cast<float>(x), static_cast<float>(y));

            if (noiseValue < -0.3f) {
                m_map[y][x] = 3; // Water
            }
            else if (noiseValue < -0.1f) {
                m_map[y][x] = 6; // Sand
            }
            else if (noiseValue < 0.3f) {
                m_map[y][x] = 1; // Grass
            }
            else if (noiseValue < 0.5f) {
                if ((x + y) % 5 == 0) {
                    m_map[y][x] = 5; // Tree
                }
                else {
                    m_map[y][x] = 1; // Grass
                }
            }
            else if (noiseValue < 0.7f) {
                m_map[y][x] = 2; // Stone wall
            }
            else {
                m_map[y][x] = 7; // Mountain
            }

            if ((x * y) % 97 == 0) {
                m_map[y][x] = 4; // Lava
            }
        }
    }

    Logger::Log("World generation completed!");
}

void World::GenerateFromConfig(const std::string& configPath) {
    Logger::Log("=== STARTING GENERATE FROM CONFIG ===");

    if (!m_genConfig.LoadFromFile(configPath)) {
        Logger::Log("ERROR: Failed to load config, using default generation");
        Generate(12345);
        return;
    }

    m_width = m_genConfig.width;
    m_height = m_genConfig.height;
    m_map.resize(m_height, std::vector<int>(m_width, 0));

    Logger::Log("World size set from config: " + std::to_string(m_width) + "x" + std::to_string(m_height));

    if (m_genConfig.useRandomSeed) {
        m_currentSeed = static_cast<int>(time(nullptr));
    }
    else {
        m_currentSeed = m_genConfig.seed;
    }

    m_noiseGenerator.SetSeed(m_currentSeed);
    m_noiseGenerator.SetFrequency(m_genConfig.noiseFrequency);

    Logger::Log("Using seed: " + std::to_string(m_currentSeed) +
        ", noise frequency: " + std::to_string(m_genConfig.noiseFrequency));

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);

    int tilesPlacedByBiomes = 0;
    int tilesPlacedByFeatures = 0;

    for (int y = 0; y < m_height; y++) {
        for (int x = 0; x < m_width; x++) {
            if (x == 0 || x == m_width - 1 || y == 0 || y == m_height - 1) {
                m_map[y][x] = 2;
                continue;
            }

            float noiseValue = m_noiseGenerator.GetNoise(static_cast<float>(x), static_cast<float>(y));

            bool tilePlaced = false;
            for (const auto& biome : m_genConfig.biomeRules) {
                const BiomeRule& rule = biome.second;
                if (noiseValue >= rule.minHeight && noiseValue < rule.maxHeight) {
                    if (dis(gen) <= rule.probability) {
                        m_map[y][x] = rule.tileId;
                        tilePlaced = true;
                        tilesPlacedByBiomes++;
                        break;
                    }
                }
            }

            if (!tilePlaced) {
                m_map[y][x] = 0;
            }

            for (const auto& feature : m_genConfig.specialFeatures) {
                if (dis(gen) <= feature.second) {
                    m_map[y][x] = feature.first;
                    tilesPlacedByFeatures++;
                    break;
                }
            }
        }
    }

    Logger::Log("Barrier created around the world");
    Logger::Log("Tiles placed by biomes: " + std::to_string(tilesPlacedByBiomes));
    Logger::Log("Tiles placed by features: " + std::to_string(tilesPlacedByFeatures));

    Logger::Log("=== GENERATE FROM CONFIG COMPLETED ===");
}

bool World::LoadFromFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        std::cout << "Failed to load world from: " << path << std::endl;
        return false;
    }

    file.read(reinterpret_cast<char*>(&m_width), sizeof(m_width));
    file.read(reinterpret_cast<char*>(&m_height), sizeof(m_height));
    file.read(reinterpret_cast<char*>(&m_currentSeed), sizeof(m_currentSeed));

    m_map.resize(m_height, std::vector<int>(m_width, 0));
    for (int y = 0; y < m_height; y++) {
        file.read(reinterpret_cast<char*>(m_map[y].data()), m_width * sizeof(int));
    }

    file.close();
    std::cout << "World loaded from: " << path << std::endl;
    return true;
}

bool World::SaveToFile(const std::string& path) {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
        std::cout << "Failed to save world to: " << path << std::endl;
        return false;
    }

    file.write(reinterpret_cast<const char*>(&m_width), sizeof(m_width));
    file.write(reinterpret_cast<const char*>(&m_height), sizeof(m_height));
    file.write(reinterpret_cast<const char*>(&m_currentSeed), sizeof(m_currentSeed));

    for (int y = 0; y < m_height; y++) {
        file.write(reinterpret_cast<const char*>(m_map[y].data()), m_width * sizeof(int));
    }

    file.close();
    std::cout << "World saved to: " << path << std::endl;
    return true;
}

int World::GetTileAt(int x, int y) const {
    if (x >= 0 && x < m_width && y >= 0 && y < m_height) {
        return m_map[y][x];
    }
    return 0;
}

void World::SetSize(int width, int height) {
    m_width = width;
    m_height = height;
    m_map.resize(m_height, std::vector<int>(m_width, 0));
}