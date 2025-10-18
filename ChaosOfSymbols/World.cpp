#include "World.h"
#include "Logger.h"
#include <random>

World::World()
    : m_width(0), m_height(0), m_currentSeed(0), m_automatonEnabled(true), m_tileManager(nullptr) {
}

void World::GenerateFromConfig(const std::string& configPath) {
    Logger::Log("=== STARTING PURE RULE-BASED GENERATION ===");

    if (!m_genConfig.LoadFromFile(configPath)) {
        Logger::Log("ERROR: Failed to load world generation config");
        return;
    }

    m_width = m_genConfig.width;
    m_height = m_genConfig.height;
    m_map.resize(m_height, std::vector<int>(m_width, 0));

    if (m_genConfig.useRandomSeed) {
        m_currentSeed = static_cast<int>(time(nullptr));
    }
    else {
        m_currentSeed = m_genConfig.seed;
    }

    m_noiseGenerator.SetSeed(m_currentSeed);
    m_noiseGenerator.SetFrequency(m_genConfig.noiseFrequency);

    Logger::Log("World size: " + std::to_string(m_width) + "x" + std::to_string(m_height));
    Logger::Log("Using seed: " + std::to_string(m_currentSeed));

    // Загружаем конфиг клеточного автомата
    if (!m_automatonConfig.LoadFromFile("config/cellular_automaton.cfg")) {
        Logger::Log("WARNING: Failed to load cellular automaton config");
    }

    // Генерируем базовый террейн
    GenerateBaseTerrain();

    // Применяем правила спавна
    if (m_spawnConfig.LoadFromFile()) {
        ApplySpawnRules();
    }

    Logger::Log("=== RULE-BASED GENERATION COMPLETED ===");
}

void World::GenerateBaseTerrain() {
    if (!m_tileManager) {
        Logger::Log("ERROR: No tile manager for base terrain generation");
        return;
    }

    // Находим ID для базового тайла (например, травы)
    int baseTileId = FindTileIdByCharacter('.');
    if (baseTileId == -1) {
        // Если травы нет, берем первый доступный тайл
        if (!m_tileManager->GetAllTiles().empty()) {
            baseTileId = m_tileManager->GetAllTiles().begin()->first;
        }
        else {
            Logger::Log("ERROR: No tiles available for base terrain");
            return;
        }
    }

    Logger::Log("Using base tile ID: " + std::to_string(baseTileId));

    // Заполняем карту базовым тайлом
    for (int y = 0; y < m_height; y++) {
        for (int x = 0; x < m_width; x++) {
            m_map[y][x] = baseTileId;
        }
    }

    Logger::Log("Base terrain generated with tile: " + std::to_string(baseTileId));
}

void World::ApplySpawnRules() {
    if (!m_tileManager) return;

    Logger::Log("Applying spawn rules");

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);

    int spawnCount = 0;

    for (int y = 0; y < m_height; y++) {
        for (int x = 0; x < m_width; x++) {
            int currentTileId = m_map[y][x];
            char currentChar = GetTileCharacter(currentTileId);

            // Получаем ID текущего базового тайла для проверки разрешенных тайлов
            int currentBaseTileId = currentTileId;

            // Проверяем все правила спавна
            const auto& allSpawnRules = m_spawnConfig.GetAllRules();
            for (const auto& spawnPair : allSpawnRules) {
                char spawnTileChar = spawnPair.first;

                // Находим ID тайла для спавна по его символу
                int spawnTileId = FindTileIdByCharacter(spawnTileChar);
                if (spawnTileId == -1) continue;

                // Проверяем все правила для этого символа спавна
                for (const auto& rule : spawnPair.second) {
                    // Проверяем, может ли этот тайл спавниться на текущем базовом тайле
                    if (rule.CanSpawnOn(currentChar) && dis(gen) <= rule.probability) {
                        m_map[y][x] = spawnTileId;
                        spawnCount++;
                        break; // только одно правило на тайл
                    }
                }
            }
        }
    }

    Logger::Log("Spawn rules applied: " + std::to_string(spawnCount) + " tiles changed");
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

    for (int y = 1; y < m_height - 1; y++) {
        for (int x = 1; x < m_width - 1; x++) {
            char currentChar = GetTileCharacter(m_map[y][x]);
            const CellRule* rule = m_automatonConfig.GetRule(currentChar);
            auto neighborCounts = CountNeighbors(x, y, m_map);

            // ПРОВЕРКА СМЕРТИ для существующих клеток
            if (m_map[y][x] != 0 && rule && rule->deathRule) {
                bool shouldDie = rule->deathRule(neighborCounts);
                if (shouldDie) {
                    newMap[y][x] = 0; // Клетка умирает
                    changed = true;
                    deaths++;
                    naturalDeaths++;
                    if (naturalDeaths <= 3) {
                        Logger::Log("NATURAL DEATH at " + std::to_string(x) + "," + std::to_string(y) +
                            " - '" + std::string(1, currentChar) + "'");
                    }
                    continue; // Переходим к следующей клетке
                }
            }

            // Старая логика выживания и рождения
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

    // Проверяем 8 соседей (окрестность Мура)
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;

            int nx = x + dx;
            int ny = y + dy;

            if (nx >= 0 && nx < m_width && ny >= 0 && ny < m_height) {
                char neighborChar = GetTileCharacter(currentMap[ny][nx]);
                counts[neighborChar]++;
            }
        }
    }

    return counts;
}

int World::GetTileAt(int x, int y) const {
    if (x >= 0 && x < m_width && y >= 0 && y < m_height) {
        return m_map[y][x];
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