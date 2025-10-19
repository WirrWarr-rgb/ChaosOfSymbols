#include "World.h"
#include "Logger.h"
#include <random>
#include <algorithm>
#include <ctime> 

World::World()
    : m_width(0), m_height(0), m_contentWidth(0), m_contentHeight(0),
    m_currentSeed(0), m_automatonEnabled(true), m_tileManager(nullptr) {
}

void World::GenerateFromConfig(const std::string& configPath) {
    Logger::Log("=== STARTING PURE RULE-BASED GENERATION ===");

    if (!m_genConfig.LoadFromFile(configPath)) {
        Logger::Log("ERROR: Failed to load world generation config");
        return;
    }

    // �������� ������� ������ - �������� ���
    if (!m_spawnConfig.LoadFromFile()) {
        Logger::Log("ERROR: Failed to load spawn config");
        return;
    }

    // ������������ �������: ������� + ������� � ���� ������
    m_contentWidth = m_genConfig.width;
    m_contentHeight = m_genConfig.height;
    m_width = m_contentWidth + 2;
    m_height = m_contentHeight + 2;

    m_map.resize(m_height, std::vector<int>(m_width, 0));

    if (m_genConfig.useRandomSeed) {
        m_currentSeed = static_cast<int>(time(nullptr));
    }
    else {
        m_currentSeed = m_genConfig.seed;
    }

    m_noiseGenerator.SetSeed(m_currentSeed);
    m_noiseGenerator.SetFrequency(m_genConfig.noiseFrequency);

    Logger::Log("Content size: " + std::to_string(m_contentWidth) + "x" + std::to_string(m_contentHeight));
    Logger::Log("Total size with border: " + std::to_string(m_width) + "x" + std::to_string(m_height));
    Logger::Log("Using seed: " + std::to_string(m_currentSeed));

    // ��������� ������ ���������� ��������
    if (!m_automatonConfig.LoadFromFile("config/cellular_automaton.cfg")) {
        Logger::Log("WARNING: Failed to load cellular automaton config");
    }

    // ���������� ������� �������
    GenerateBaseTerrain();

    // ������� ������� �� �������� #
    CreateBorder();

    SmoothTerrain();


    Logger::Log("=== RULE-BASED GENERATION COMPLETED ===");
}

void World::GenerateBaseTerrain() {
    if (!m_tileManager) {
        Logger::Log("ERROR: No tile manager for base terrain generation");
        return;
    }

    Logger::Log("Generating terrain based on height zones...");

    const auto& spawnRules = m_spawnConfig.GetAllRules();

    if (spawnRules.empty()) {
        Logger::Log("WARNING: No spawn rules found");
        return;
    }

    // ���������� ������� ����������� �� �������
    char waterChar = FindWaterTile(spawnRules);
    char grassChar = FindGrassTile(spawnRules);
    char mountainChar = FindMountainTile(spawnRules);

    Logger::Log("Detected tiles - Water: '" + std::string(1, waterChar) +
        "', Grass: '" + std::string(1, grassChar) +
        "', Mountain: '" + std::string(1, mountainChar) + "'");

    int tilesPlaced = 0;
    std::unordered_map<char, int> tileStatistics;

    for (int y = 1; y < m_height - 1; y++) {
        for (int x = 1; x < m_width - 1; x++) {
            // ����� ������� ������� ���� ��� �������� ����
            float baseNoise = (m_noiseGenerator.GetNoise((float)x * 0.03f, (float)y * 0.03f) + 1.0f) * 0.5f;
            float ridgeNoise = 1.0f - std::abs(m_noiseGenerator.GetNoise((float)x * 0.06f + 1000, (float)y * 0.06f + 1000));
            float detailNoise = (m_noiseGenerator.GetNoise((float)x * 0.15f + 2000, (float)y * 0.15f + 2000) + 1.0f) * 0.5f;

            // ����������� ����
            float heightNoise = baseNoise * 0.4f + ridgeNoise * 0.4f + detailNoise * 0.2f;

            // ������ ����, ������ ������������
            heightNoise = std::pow(heightNoise, 1.1f); // ������ ������

            // ����� ���� ��� �������� ������������
            int zone;
            if (heightNoise < 0.25f) {  // ������ 25% ��� ���� (���� 30%)
                zone = 0; // ������ ���� (����)
            }
            else if (heightNoise < 0.7f) { // 45% ��� �����
                zone = 1; // ������� ���� (�����)
            }
            else { // 30% ��� ���
                zone = 2; // ������� ���� (����)
            }

            // �������� ���� �� ������ ����
            char selectedTile = SelectTileByZone(zone, spawnRules, x, y);
            int selectedTileId = FindTileIdByCharacter(selectedTile);

            if (selectedTileId != -1) {
                m_map[y][x] = selectedTileId;
                tilesPlaced++;
                tileStatistics[selectedTile]++;
            }
        }
    }

    // �������� ���������� � ������� ������
    Logger::Log("Zone-based terrain generated: " + std::to_string(tilesPlaced) + " tiles placed");
    for (const auto& stat : tileStatistics) {
        std::string tileName = "unknown";
        TileType* tile = m_tileManager->GetTileType(FindTileIdByCharacter(stat.first));
        if (tile) {
            tileName = tile->GetName();
        }
        Logger::Log("  '" + std::string(1, stat.first) + "' (" + tileName + "): " + std::to_string(stat.second));
    }
}

void World::SmoothTerrain() {
    if (!m_tileManager) return;

    Logger::Log("Smoothing terrain with natural transitions...");

    const auto& spawnRules = m_spawnConfig.GetAllRules();
    if (spawnRules.empty()) return;

    char waterChar = FindWaterTile(spawnRules);
    char grassChar = FindGrassTile(spawnRules);
    char mountainChar = FindMountainTile(spawnRules);

    std::vector<std::vector<int>> newMap = m_map;
    int changes = 0;

    for (int y = 1; y < m_height - 1; y++) {
        for (int x = 1; x < m_width - 1; x++) {
            auto neighbors = CountNeighbors(x, y, m_map);
            char current = GetTileCharacter(m_map[y][x]);

            int waterCount = neighbors.count(waterChar) ? neighbors.at(waterChar) : 0;
            int mountainCount = neighbors.count(mountainChar) ? neighbors.at(mountainChar) : 0;
            int grassCount = neighbors.count(grassChar) ? neighbors.at(grassChar) : 0;

            // ������������ �������� ����� �������
            if (current == mountainChar) {
                if (waterCount >= 4) {
                    newMap[y][x] = FindTileIdByCharacter(waterChar); // ���� � ���� -> ����
                    changes++;
                }
                else if (waterCount >= 3 && grassCount <= 2) {
                    newMap[y][x] = FindTileIdByCharacter(waterChar); // ���� ����� � ����� -> ����
                    changes++;
                }
            }
            else if (current == waterChar) {
                if (mountainCount >= 5) {
                    newMap[y][x] = FindTileIdByCharacter(mountainChar); // ���� � ����� -> ����
                    changes++;
                }
                else if (grassCount >= 6 && mountainCount <= 1) {
                    newMap[y][x] = FindTileIdByCharacter(grassChar); // ���������� -> �����
                    changes++;
                }
            }
            else if (current == grassChar) {
                if (waterCount >= 5) {
                    newMap[y][x] = FindTileIdByCharacter(waterChar); // ������������ ����� -> ����
                    changes++;
                }
                else if (mountainCount >= 4 && waterCount <= 1) {
                    newMap[y][x] = FindTileIdByCharacter(mountainChar); // ��������� -> ����
                    changes++;
                }
            }
        }
    }

    if (changes > 0) {
        m_map = newMap;
        Logger::Log("Natural smoothing applied: " + std::to_string(changes) + " changes made");
    }
}

char World::SelectTileByZone(int zone, const std::unordered_map<char, SpawnRule>& spawnRules, int x, int y) {
    // ���������� ���������� ��� ������������������, �� �������� ������
    float noise = (m_noiseGenerator.GetNoise((float)x * 0.1f, (float)y * 0.1f) + 1.0f) * 0.5f;

    // ������� ���������� ����� �� ������ ����
    std::vector<std::pair<char, float>> weightedTiles;

    for (const auto& pair : spawnRules) {
        char tileChar = pair.first;
        const SpawnRule& rule = pair.second;

        if (rule.zoneProbabilities.size() > zone) {
            float baseProb = rule.zoneProbabilities[zone];
            // ��������� ��������� ������������� �� ������ ����
            float variedProb = baseProb * (0.9f + noise * 0.2f);
            weightedTiles.push_back({ tileChar, variedProb });
        }
    }

    // ��������� �� ����������� (�� ������� � ������)
    std::sort(weightedTiles.begin(), weightedTiles.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });

    // ���������� ����� ��������� ����
    if (!weightedTiles.empty()) {
        return weightedTiles[0].first;
    }

    return '.';
}

float World::GetProbabilityForZone(const SpawnRule& rule, int zone) {
    if (zone >= 0 && zone < rule.zoneProbabilities.size()) {
        return rule.zoneProbabilities[zone];
    }
    return 0.1f; // fallback
}

void World::CreateBorder() {
    if (!m_tileManager) {
        Logger::Log("ERROR: No tile manager for border creation");
        return;
    }

    // ������� ID ��� ����� ������� (#)
    int borderTileId = FindTileIdByCharacter('#');
    if (borderTileId == -1) {
        Logger::Log("WARNING: Border tile '#' not found, skipping border creation");
        return;
    }

    Logger::Log("Creating border with tile ID: " + std::to_string(borderTileId));

    // ������� ������� �� ��������� �����
    for (int x = 0; x < m_width; x++) {
        // ������� ������� (y = 0)
        m_map[0][x] = borderTileId;
        // ������ ������� (y = m_height - 1)
        m_map[m_height - 1][x] = borderTileId;
    }

    for (int y = 0; y < m_height; y++) {
        // ����� ������� (x = 0)
        m_map[y][0] = borderTileId;
        // ������ ������� (x = m_width - 1)
        m_map[y][m_width - 1] = borderTileId;
    }

    Logger::Log("Border created successfully");
}

void World::UpdateCellularAutomaton() {
    if (!m_automatonEnabled || !m_tileManager) {
        Logger::Log("Cellular automaton disabled or no tile manager");
        return;
    }

    Logger::Log("=== STARTING CELLULAR AUTOMATON UPDATE ===");

    if (m_automatonConfig.GetAllRules().empty()) {
        Logger::Log("ERROR: No cellular automaton rules available!");
        return;
    }

    std::vector<std::vector<int>> newMap = m_map;
    bool changed = false;
    int deaths = 0;
    int births = 0;
    int naturalDeaths = 0;

    // ��������� ������� �������� ������ � ������� ������� (��� �������)
    for (int y = 1; y < m_height - 1; y++) {
        for (int x = 1; x < m_width - 1; x++) {
            if (x == 0 || x == m_width - 1 || y == 0 || y == m_height - 1) {
                continue;
            }

            char currentChar = GetTileCharacter(m_map[y][x]);
            const CellRule* rule = m_automatonConfig.GetRule(currentChar);
            auto neighborCounts = CountNeighbors(x, y, m_map);

            //// ���������� ��������� ����� (#), ���� ��� �� ������ ���� �����
            //if (currentChar == '#') {
            //    continue;
            //}

            // �������� ������ ��� ������������ ������
            if (m_map[y][x] != 0 && rule && rule->deathRule) {
                bool shouldDie = rule->deathRule(neighborCounts);
                if (shouldDie) {
                    newMap[y][x] = 0; // ������ �������
                    changed = true;
                    deaths++;
                    naturalDeaths++;
                    if (naturalDeaths <= 3) {
                        Logger::Log("NATURAL DEATH at " + std::to_string(x) + "," + std::to_string(y) +
                            " - '" + std::string(1, currentChar) + "'");
                    }
                    continue; // ��������� � ��������� ������
                }
            }

            // ������ ������ ��������� � ��������
            if (m_map[y][x] != 0) {
                if (rule && rule->survivalRule) {
                    bool shouldSurvive = rule->survivalRule(neighborCounts);
                    if (!shouldSurvive) {
                        newMap[y][x] = 0;
                        changed = true;
                        deaths++;
                    }
                }
            }
            else {
                const auto& allRules = m_automatonConfig.GetAllRules();
                for (auto it = allRules.begin(); it != allRules.end(); ++it) {
                    char tileChar = it->first;
                    const CellRule& birthRule = it->second;

                    if (birthRule.birthRule && birthRule.birthRule(neighborCounts)) {
                        int newTileId = FindTileIdByCharacter(tileChar);
                        if (newTileId != -1) {
                            newMap[y][x] = newTileId;
                            changed = true;
                            births++;
                            break;
                        }
                    }
                }
            }
        }
    }

    if (changed) {
        m_map = newMap;
        Logger::Log("Cellular automaton: " + std::to_string(births) + " births, " +
            std::to_string(deaths) + " deaths (" + std::to_string(naturalDeaths) + " natural)");
    }

    Logger::Log("=== CELLULAR AUTOMATON UPDATE COMPLETE ===");
}

std::unordered_map<char, int> World::CountNeighbors(int x, int y, const std::vector<std::vector<int>>& currentMap) const {
    std::unordered_map<char, int> counts;

    // ��������� 24 ������ (����������� 5x5 ��� ������)
    for (int dy = -2; dy <= 2; dy++) {
        for (int dx = -2; dx <= 2; dx++) {
            if (dx == 0 && dy == 0) continue; // ���������� ���� ������

            int nx = x + dx;
            int ny = y + dy;

            // ���������� ��������� ������
            if (nx <= 0 || nx >= m_width - 1 || ny <= 0 || ny >= m_height - 1) {
                continue;
            }

            if (nx >= 0 && nx < m_width && ny >= 0 && ny < m_height) {
                char neighborChar = GetTileCharacter(currentMap[ny][nx]);
                counts[neighborChar]++;
            }
        }
    }

    return counts;
}

int World::GetTileAt(int x, int y) const {
    // ���������� ���������� ��� ������� ������� (0,0 - ����� ������� ���� �������� ������������)
    // ����������� � ���������� ������ ����� (� ��������)
    int mapX = x + 1;
    int mapY = y + 1;

    if (mapX >= 0 && mapX < m_width && mapY >= 0 && mapY < m_height) {
        return m_map[mapY][mapX];
    }
    return 0;
}

char World::GetTileCharacter(int tileId) const {
    if (!m_tileManager) return '.';
    TileType* tile = m_tileManager->GetTileType(tileId);
    return tile ? tile->GetCharacter() : '.';
}

int World::FindTileIdByCharacter(char character) const {
    if (!m_tileManager) return -1;

    const auto& allTiles = m_tileManager->GetAllTiles();
    for (const auto& pair : allTiles) {
        const TileType& tile = pair.second;
        if (tile.GetCharacter() == character) {
            return tile.GetId();
        }
    }
    return -1;
}

char World::FindWaterTile(const std::unordered_map<char, SpawnRule>& spawnRules) const {
    return FindTileByTerrainType("water", spawnRules);
}

char World::FindGrassTile(const std::unordered_map<char, SpawnRule>& spawnRules) const {
    return FindTileByTerrainType("grass", spawnRules);
}

char World::FindMountainTile(const std::unordered_map<char, SpawnRule>& spawnRules) const {
    return FindTileByTerrainType("mountain", spawnRules);
}

char World::FindTileByTerrainType(const std::string& terrainType, const std::unordered_map<char, SpawnRule>& spawnRules) const {
    if (!m_tileManager) return '?';

    // ������� �������� ����� �� ����� � TileType
    const auto& allTiles = m_tileManager->GetAllTiles();
    for (const auto& pair : allTiles) {
        const TileType& tile = pair.second;
        std::string name = tile.GetName();
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);

        if (name.find(terrainType) != std::string::npos) {
            return tile.GetCharacter();
        }
    }

    // ���� �� ����� �� �����, ����������� ����������� �� �����-������
    for (const auto& spawnPair : spawnRules) {
        char character = spawnPair.first;
        const SpawnRule& rule = spawnPair.second;

        if (rule.zoneProbabilities.size() >= 3) {
            float lowProb = rule.zoneProbabilities[0];  // ������
            float midProb = rule.zoneProbabilities[1];  // �������
            float highProb = rule.zoneProbabilities[2]; // ����

            if (terrainType == "water" && lowProb > midProb && lowProb > highProb) {
                return character;
            }
            else if (terrainType == "grass" && midProb > lowProb && midProb > highProb) {
                return character;
            }
            else if (terrainType == "mountain" && highProb > lowProb && highProb > midProb) {
                return character;
            }
        }
    }

    // Fallback: ���������� ������ ������ �� ������
    return spawnRules.empty() ? '?' : spawnRules.begin()->first;
}