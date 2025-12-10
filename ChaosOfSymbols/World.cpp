#include <random>
#include <algorithm>
#include <ctime>
#include <filesystem>
#include "World.h"
#include "Logger.h"

World::World()
    : m_width(0), m_height(0), m_contentWidth(0), m_contentHeight(0),
    m_automatonEnabled(true), m_tileManager(nullptr), m_foodManager(nullptr),
    m_automatonConfig(nullptr)
{
}

/// <summary>
/// Генерация мира согласно конфигу
/// </summary>
void World::GenerateFromConfig() {
    Logger::Log("\n=== STARTING WORLD GENERATION ===\n");

    Logger::Log("WorldConfig settings:");
    Logger::Log("  Width: " + std::to_string(m_config.GetWidth()));
    Logger::Log("  Height: " + std::to_string(m_config.GetHeight()));
    Logger::Log("  Seed: " + std::to_string(m_config.GetEffectiveSeed()));
    Logger::Log("  NoiseFrequency: " + std::to_string(m_config.GetNoiseFrequency()));
    Logger::Log("  GenerationMode: " + std::to_string(static_cast<int>(m_config.GetGenerationMode())));

    if (!m_config.LoadConfig()) {
        Logger::Log("ERROR: Failed to load world generation config");
        Logger::Log("Attempting generation with current settings...");
    }

    switch (m_config.GetGenerationMode()) {
    case WorldGenerationMode::FROM_MAP_FILE:
        if (m_config.GetMapFilePath().empty() || !std::filesystem::exists(m_config.GetMapFilePath())) {
            Logger::Log("Map file not found or not specified: " + m_config.GetMapFilePath());
            Logger::Log("Falling back to procedural generation with save parameters");
            GenerateRandomWorld();
        }
        else {
            Logger::Log("Generating from map file: " + m_config.GetMapFilePath());
            if (!LoadMapFromFile(m_config.GetMapFilePath())) {
                Logger::Log("ERROR: Failed to load map from file: " + m_config.GetMapFilePath());
                Logger::Log("Falling back to random generation with save parameters");
                GenerateRandomWorld();
            }
            else {
                Logger::Log("=== MAP LOADED FROM FILE ===");
                return;
            }
        }
        break;

    case WorldGenerationMode::SEEDED:
    case WorldGenerationMode::RANDOM:
        Logger::Log("Generating random world with save parameters...");
        GenerateRandomWorld();
        break;
    }

    Logger::Log("=== WORLD GENERATION COMPLETED ===");
}

/// <summary>
/// Генерация случайного мира (общая логика для RANDOM и SEEDED)
/// </summary>
void World::GenerateRandomWorld() {
    Logger::Log("Generating random world...");

    if (!m_tileManager || m_tileManager->GetAllTiles().empty()) {
        Logger::Log("WARNING: No tiles available for world generation!");
        Logger::Log("Creating minimal essential tiles...");

        if (m_tileManager) {
            m_tileManager->RegisterTileType(TileType(0, "air", ' ', 0, true, false, 0, 0, 0, 0));
            m_tileManager->RegisterTileType(TileType(1, "grass", '.', 10, true, false, 0, 0, 100, 0));
            m_tileManager->RegisterTileType(TileType(2, "border", '#', 8, false, true, 0, 0, 0, 0));

            Logger::Log("Created minimal tile set for world generation");
        }
    }

    m_contentWidth = m_config.GetWidth();
    m_contentHeight = m_config.GetHeight();
    m_width = m_contentWidth + 2;
    m_height = m_contentHeight + 2;

    m_map.resize(m_height, std::vector<int>(m_width, 0));

    // ВАЖНО: Используем GetEffectiveSeed(), который сам решит, какой seed использовать
    int currentSeed = m_config.GetEffectiveSeed();

    m_noiseGenerator.SetSeed(currentSeed);
    m_noiseGenerator.SetFrequency(m_config.GetNoiseFrequency());

    std::string modeStr;
    switch (m_config.GetGenerationMode()) {
    case WorldGenerationMode::RANDOM:
        modeStr = "RANDOM";
        break;
    case WorldGenerationMode::SEEDED:
        modeStr = "SEEDED";
        break;
    default:
        modeStr = "UNKNOWN";
        break;
    }

    Logger::Log("Content size: " + std::to_string(m_contentWidth) + "x" + std::to_string(m_contentHeight));
    Logger::Log("Total size with border: " + std::to_string(m_width) + "x" + std::to_string(m_height));
    Logger::Log("Generation mode: " + modeStr);
    Logger::Log("Using seed: " + std::to_string(currentSeed));

    if (m_automatonConfig) {
        Logger::Log("Cellular automaton config is available (external)");
    }
    else {
        Logger::Log("WARNING: No cellular automaton config available");
    }

    GenerateBaseTerrain();
    CreateBorder();
    SmoothTerrain();

    if (m_foodManager) {
        int initialFoodCount = (m_contentWidth * m_contentHeight) / 10;
        initialFoodCount = std::min(initialFoodCount, 30);
        SpawnRandomFood(initialFoodCount);
    }
    else {
        Logger::Log("WARNING: No food manager available for initial food spawn");
    }

    Logger::Log("=== WORLD GENERATION COMPLETED ===");
}

/// <summary>
/// Загрузка карты из файла
/// </summary>
bool World::LoadMapFromFile(const std::string& filePath) {
    Logger::Log("Loading map from file: " + filePath);

    std::ifstream file(filePath);
    if (!file.is_open()) {
        Logger::Log("ERROR: Cannot open map file: " + filePath);
        return false;
    }

    std::vector<std::string> lines;
    std::string line;
    int maxWidth = 0;

    while (std::getline(file, line)) {
        if (line.empty() || (line.length() >= 2 && line[0] == '/' && line[1] == '/')) {
            continue;
        }

        lines.push_back(line);
        if (line.length() > maxWidth) {
            maxWidth = line.length();
        }
    }
    file.close();

    if (lines.empty()) {
        Logger::Log("ERROR: Map file is empty or contains no valid data");
        return false;
    }

    m_contentHeight = lines.size();
    m_contentWidth = maxWidth;
    m_width = m_contentWidth + 2;
    m_height = m_contentHeight + 2;

    Logger::Log("Loaded map size: " + std::to_string(m_contentWidth) + "x" + std::to_string(m_contentHeight));

    m_map.resize(m_height, std::vector<int>(m_width, 0));

    for (int y = 0; y < m_contentHeight; y++) {
        const std::string& currentLine = lines[y];
        for (int x = 0; x < m_contentWidth; x++) {
            char mapChar = (x < currentLine.length()) ? currentLine[x] : ' ';
            int tileId = FindTileIdByCharacter(mapChar);

            if (tileId == -1) {
                Logger::Log("WARNING: Unknown character '" + std::string(1, mapChar) +
                    "' at position " + std::to_string(x) + "," + std::to_string(y) +
                    ", using default tile");
                tileId = FindTileIdByCharacter('.');
            }

            m_map[y + 1][x + 1] = tileId;
        }
    }

    CreateBorder();

    if (m_foodManager) {
        int initialFoodCount = (m_contentWidth * m_contentHeight) / 15;
        initialFoodCount = std::min(initialFoodCount, 20);
        SpawnRandomFood(initialFoodCount);
    }

    Logger::Log("Map loaded successfully from: " + filePath);
    return true;
}

/// <summary>
/// Генерация баззового пространства с шумом Перлина и правил спавна
/// </summary>
void World::GenerateBaseTerrain() {
    if (!m_tileManager) {
        Logger::Log("ERROR: No tile manager for base terrain generation");
        return;
    }

    if (m_config.GetAllSpawnRules().empty()) {
        Logger::Log("No spawn rules found, calculating from tiles...");
        // Нужно иметь доступ к WorldConfig для вызова CalculateSpawnRulesFromTiles
        // Или делаем это в WorldConfig перед генерацией
    }

    const auto& spawnRules = m_config.GetAllSpawnRules();

    Logger::Log("Spawn rules count: " + std::to_string(spawnRules.size()));
    Logger::Log("Tile manager has " + std::to_string(m_tileManager->GetAllTiles().size()) + " tiles");

    const auto& allTiles = m_tileManager->GetAllTiles();
    for (const auto& pair : allTiles) {
        const TileType& tile = pair.second;
        Logger::Log("Tile " + std::to_string(tile.GetId()) + ": '" +
            std::string(1, tile.GetCharacter()) + "' - " + tile.GetName() +
            " L:" + std::to_string(tile.GetLowlandProbability()) +
            " P:" + std::to_string(tile.GetPlainsProbability()) +
            " M:" + std::to_string(tile.GetMountainProbability()));
    }

    if (spawnRules.empty()) {
        Logger::Log("WARNING: No spawn rules found");
        return;
    }

    char waterChar = FindWaterTile(spawnRules);
    char grassChar = FindGrassTile(spawnRules);
    char mountainChar = FindMountainTile(spawnRules);

    int tilesPlaced = 0;
    std::unordered_map<char, int> tileStatistics;

    for (int y = 1; y < m_height - 1; y++) {
        for (int x = 1; x < m_width - 1; x++) {
            float baseNoise = (m_noiseGenerator.GetNoise((float)x * 0.03f, (float)y * 0.03f) + 1.0f) * 0.5f;
            float ridgeNoise = 1.0f - std::abs(m_noiseGenerator.GetNoise((float)x * 0.06f + 1000, (float)y * 0.06f + 1000));
            float detailNoise = (m_noiseGenerator.GetNoise((float)x * 0.15f + 2000, (float)y * 0.15f + 2000) + 1.0f) * 0.5f;

            float heightNoise = baseNoise * 0.4f + ridgeNoise * 0.4f + detailNoise * 0.2f;

            heightNoise = std::pow(heightNoise, 1.1f); // Меньше эрозии

            int zone;
            if (heightNoise < 0.25f) {  // 25%
                zone = 0; // Низкая зона
            }
            else if (heightNoise < 0.7f) { // 45% 
                zone = 1; // Средняя зона
            }
            else { // 30%
                zone = 2; // Высокая зона
            }

            // Выбираем тайл на основе зоны
            char selectedTile = SelectTileByZone(zone, spawnRules, x, y);
            int selectedTileId = FindTileIdByCharacter(selectedTile);

            if (selectedTileId != -1) {
                m_map[y][x] = selectedTileId;
                tilesPlaced++;
                tileStatistics[selectedTile]++;
            }
        }
    }

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

/// <summary>
/// Сглаживание пространства для естественных переходов между зонами
/// </summary>
void World::SmoothTerrain() {
    if (!m_tileManager) return;

    Logger::Log("Smoothing terrain with natural transitions...");

    const auto& spawnRules = m_config.GetAllSpawnRules();
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

            if (current == mountainChar) {
                if (waterCount >= 4) {
                    newMap[y][x] = FindTileIdByCharacter(waterChar); // Горы у воды -> вода
                    changes++;
                }
                else if (waterCount >= 3 && grassCount <= 2) {
                    newMap[y][x] = FindTileIdByCharacter(waterChar); // Горы рядом с водой -> вода
                    changes++;
                }
            }
            else if (current == waterChar) {
                if (mountainCount >= 5) {
                    newMap[y][x] = FindTileIdByCharacter(mountainChar); // Вода в горах -> горы
                    changes++;
                }
                else if (grassCount >= 6 && mountainCount <= 1) {
                    newMap[y][x] = FindTileIdByCharacter(grassChar); // Мелководье -> трава
                    changes++;
                }
            }
            else if (current == grassChar) {
                if (waterCount >= 5) {
                    newMap[y][x] = FindTileIdByCharacter(waterChar); // Заболоченная трава -> вода
                    changes++;
                }
                else if (mountainCount >= 4 && waterCount <= 1) {
                    newMap[y][x] = FindTileIdByCharacter(mountainChar); // Предгорье -> горы
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

/// <summary>
/// Выбирает тайл для зоны на основе вероятностей из спавн-правил
/// </summary>
/// <param name="zone">0=низины(вода), 1=равнины(трава), 2=горы</param>
/// <param name="spawnRules">правила спавна для разных типов terrain</param>
/// <returns>символ выбранного тайла</returns>
char World::SelectTileByZone(int zone, const std::unordered_map<char, SpawnRule>& spawnRules, int x, int y) {
    if (m_tileManager) {
        std::vector<std::pair<char, int>> tileProbabilities;
        const auto& allTiles = m_tileManager->GetAllTiles();

        // Собираем все тайлы с вероятностями для данной зоны
        for (const auto& pair : allTiles) {
            const TileType& tile = pair.second;
            char character = tile.GetCharacter();

            // Проверяем, есть ли правило спавна для этого тайла
            auto ruleIt = spawnRules.find(character);
            if (ruleIt != spawnRules.end()) {
                const SpawnRule& rule = ruleIt->second;
                if (rule.zoneProbabilities.size() > zone) {
                    int probability = static_cast<int>(rule.zoneProbabilities[zone]);
                    if (probability > 0) {
                        tileProbabilities.push_back({ character, probability });
                    }
                }
            }
        }

        if (!tileProbabilities.empty()) {
            int totalProbability = 0;
            for (const auto& tp : tileProbabilities) {
                totalProbability += tp.second;
            }

            if (totalProbability > 0) {
                // ИСПРАВЛЕНО: используем GetEffectiveSeed() вместо GetSeed()
                unsigned int hash = (x * 73856093) ^ (y * 19349663) ^ m_config.GetEffectiveSeed();
                int randomValue = hash % totalProbability;

                int cumulativeProbability = 0;
                for (const auto& tp : tileProbabilities) {
                    cumulativeProbability += tp.second;
                    if (randomValue < cumulativeProbability) {
                        Logger::Log("Selected tile '" + std::string(1, tp.first) +
                            "' from manager for zone " + std::to_string(zone) +
                            " at " + std::to_string(x) + "," + std::to_string(y) +
                            " (prob: " + std::to_string(tp.second) + "/" + std::to_string(totalProbability) + ")");
                        return tp.first;
                    }
                }
            }
        }
    }

    Logger::Log("WARNING: No tiles available for zone " + std::to_string(zone) +
        ", using default '.'");
    return '.';
}

/// <summary>
/// Создание непроходимой границы по краям карты
/// </summary>
void World::CreateBorder() {
    if (!m_tileManager) {
        Logger::Log("ERROR: No tile manager for border creation");
        return;
    }

    int borderTileId = FindTileIdByCharacter('#');
    if (borderTileId == -1) {
        Logger::Log("WARNING: Border tile '#' not found, skipping border creation");
        return;
    }

    Logger::Log("Creating border with tile ID: " + std::to_string(borderTileId));

    for (int x = 0; x < m_width; x++) {
        m_map[0][x] = borderTileId;
        m_map[m_height - 1][x] = borderTileId;
    }

    for (int y = 0; y < m_height; y++) {
        m_map[y][0] = borderTileId;
        m_map[y][m_width - 1] = borderTileId;
    }

    Logger::Log("Border created successfully");
}

/// <summary>
/// Находит символ тайла воды
/// </summary>
char World::FindWaterTile(const std::unordered_map<char, SpawnRule>& spawnRules) const {
    return FindTileByTerrainType("water", spawnRules);
}

/// <summary>
/// Находит символ тайла травы
/// </summary>
char World::FindGrassTile(const std::unordered_map<char, SpawnRule>& spawnRules) const {
    return FindTileByTerrainType("grass", spawnRules);
}

/// <summary>
/// Находит символ тайла гор
/// </summary>
char World::FindMountainTile(const std::unordered_map<char, SpawnRule>& spawnRules) const {
    return FindTileByTerrainType("mountain", spawnRules);
}

/// <summary>
/// Универсальный метод поиска тайла по типу terrain (water/grass/mountain)
/// </summary>
char World::FindTileByTerrainType(const std::string& terrainType, const std::unordered_map<char, SpawnRule>& spawnRules) const {
    if (!m_tileManager) return '?';

    Logger::Log("Looking for terrain type: " + terrainType);

    const auto& allTiles = m_tileManager->GetAllTiles();
    for (const auto& pair : allTiles) {
        const TileType& tile = pair.second;
        std::string name = tile.GetName();
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);

        if (name.find(terrainType) != std::string::npos) {
            return tile.GetCharacter();
        }
    }

    for (const auto& spawnPair : spawnRules) {
        char character = spawnPair.first;
        const SpawnRule& rule = spawnPair.second;

        if (rule.zoneProbabilities.size() >= 3) {
            float lowProb = rule.zoneProbabilities[0];
            float midProb = rule.zoneProbabilities[1];
            float highProb = rule.zoneProbabilities[2];

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

    // Fallback: возвращаем первый символ из правил
    return spawnRules.empty() ? '?' : spawnRules.begin()->first;
}

/// <summary>
/// Применение правил клеточного автомата для изменения мира
/// </summary>
void World::UpdateCellularAutomaton() {
    if (!m_automatonEnabled || !m_tileManager || !m_automatonConfig) {
        Logger::Log("Cellular automaton disabled or no config");
        return;
    }

    if (m_automatonConfig->GetAllRules().empty()) {
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
            if (x == 0 || x == m_width - 1 || y == 0 || y == m_height - 1) {
                continue;
            }

            char currentChar = GetTileCharacter(m_map[y][x]);
            const CellRule* rule = m_automatonConfig->GetRule(currentChar);
            auto neighborCounts = CountNeighbors(x, y, m_map);

            if (m_map[y][x] != 0 && rule && rule->deathRule) {
                bool shouldDie = rule->deathRule->evaluate(neighborCounts);
                if (shouldDie) {
                    newMap[y][x] = 0;
                    changed = true;
                    deaths++;
                    naturalDeaths++;
                    if (naturalDeaths <= 3) {
                        Logger::Log("NATURAL DEATH at " + std::to_string(x) + "," + std::to_string(y) +
                            " - '" + std::string(1, currentChar) + "'");
                    }
                    continue;
                }
            }

            if (m_map[y][x] != 0) {
                if (rule && rule->survivalRule) {
                    bool shouldSurvive = rule->survivalRule->evaluate(neighborCounts);
                    if (!shouldSurvive) {
                        newMap[y][x] = 0;
                        changed = true;
                        deaths++;
                    }
                }
            }
            else {
                const auto& allRules = m_automatonConfig->GetAllRules();
                for (auto it = allRules.begin(); it != allRules.end(); ++it) {
                    char tileChar = it->first;
                    const CellRule& birthRule = it->second;

                    if (birthRule.birthRule && birthRule.birthRule->evaluate(neighborCounts)) {
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

/// <summary>
/// Подсчет соседей каждого типа вокруг клетки
/// </summary>
std::unordered_map<char, int> World::CountNeighbors(int x, int y, const std::vector<std::vector<int>>& currentMap) const {
    std::unordered_map<char, int> counts;

    int radius = m_config.GetNeighborRadius();

    // Окрестность фон Неймана
    if (radius == 0) {
        CheckNeighbor(x, y, -1, 0, currentMap, counts);
        CheckNeighbor(x, y, 1, 0, currentMap, counts); 
        CheckNeighbor(x, y, 0, -1, currentMap, counts);
        CheckNeighbor(x, y, 0, 1, currentMap, counts);
    }
    else { // Окресть Мура
        for (int dy = -radius; dy <= radius; dy++) {
            for (int dx = -radius; dx <= radius; dx++) {
                if (dx == 0 && dy == 0) continue;
                CheckNeighbor(x, y, dx, dy, currentMap, counts);
            }
        }
    }

    return counts;
}

/// <summary>
/// Вспомогательный метод для проверки одного соседа
/// </summary>
void World::CheckNeighbor(int x, int y, int dx, int dy,
    const std::vector<std::vector<int>>& currentMap,
    std::unordered_map<char, int>& counts) const {
    int nx = x + dx;
    int ny = y + dy;

    if (nx <= 0 || nx >= m_width - 1 || ny <= 0 || ny >= m_height - 1) {
        return;
    }

    if (nx >= 0 && nx < m_width && ny >= 0 && ny < m_height) {
        char neighborChar = GetTileCharacter(currentMap[ny][nx]);
        counts[neighborChar]++;
    }
}

/// <summary>
/// Возвращение ID тайла в игровых координатах (без учета границы)
/// </summary>
/// <param name="x"></param>
/// <param name="y"></param>
/// <returns></returns>
int World::GetTileAt(int x, int y) const {
    // Координаты передаются для игровой области (0,0 - левый верхний угол игрового пространства)
    int mapX = x + 1;
    int mapY = y + 1;

    if (mapX >= 0 && mapX < m_width && mapY >= 0 && mapY < m_height) {
        return m_map[mapY][mapX];
    }
    return 0;
}

/// <summary>
/// Преобразование ID тайла в символ для отображения
/// </summary>
char World::GetTileCharacter(int tileId) const {
    if (!m_tileManager) return '.';
    TileType* tile = m_tileManager->GetTileType(tileId);
    return tile ? tile->GetCharacter() : '.';
}

/// <summary>
/// Находит ID тайла по его символу
/// </summary>
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

/// <summary>
/// Обновление внешнего вида всех тайлов после изменений конфигураций
/// </summary>
void World::UpdateTileAppearance() {
    if (!m_tileManager) return;

    Logger::Log("Updating tile appearances...");
    int changes = 0;

    for (int y = 1; y < m_height - 1; y++) {
        for (int x = 1; x < m_width - 1; x++) {
            int tileId = m_map[y][x];
            TileType* tile = m_tileManager->GetTileType(tileId);

            if (!tile) {
                m_map[y][x] = GetTileCharacter(0);
                changes++;
            }
        }
    }

    if (changes > 0) {
        Logger::Log("Updated " + std::to_string(changes) + " tile appearances");
    }
}

/// <summary>
/// Замена удаленных тайлов на тайлы по умолчанию
/// </summary>
void World::RemoveDeletedTiles(const std::unordered_set<int>& removedTileIds) {
    if (removedTileIds.empty()) return;

    Logger::Log("Removing deleted tiles from world...");
    int replacements = 0;

    for (int y = 1; y < m_height - 1; y++) {
        for (int x = 1; x < m_width - 1; x++) {
            if (removedTileIds.find(m_map[y][x]) != removedTileIds.end()) {
                m_map[y][x] = GetTileCharacter(0);
                replacements++;
            }
        }
    }

    if (replacements > 0) {
        Logger::Log("Replaced " + std::to_string(replacements) + " deleted tiles with grass");
    }
}

/// <summary>
/// Генерация случайной еды в проходимых местах карты
/// </summary>
void World::SpawnRandomFood(int count) {
    if (!m_foodManager || !m_tileManager) {
        Logger::Log("WARNING: Cannot spawn food - no food manager or tile manager");
        return;
    }

    Logger::Log("Attempting to spawn " + std::to_string(count) +
        " food items using current food manager");

    // Проверяем, сколько еды доступно в менеджере
    const auto& allFoods = m_foodManager->GetAllFood();
    Logger::Log("Food manager has " + std::to_string(allFoods.size()) + " food types");

    for (const auto& food : allFoods) {
        if (food) {
            Logger::Log("  - " + food->GetName() +
                " (weight: " + std::to_string(food->GetSpawnWeight()) +
                ", XP: " + std::to_string(food->GetExperience()) + ")");
        }
    }

    int spawned = 0;
    int attempts = 0;
    const int maxAttempts = count * 20;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> disX(0, m_contentWidth - 1);
    std::uniform_int_distribution<> disY(0, m_contentHeight - 1);

    while (spawned < count && attempts < maxAttempts) {
        int x = disX(gen);
        int y = disY(gen);

        if (CanSpawnFoodAt(x, y)) {
            const Food* food = m_foodManager->GetRandomFood();
            if (food) {
                int key = y * m_contentWidth + x;
                m_foodSpawns[key] = { x, y, food->GetId() };
                spawned++;

                Logger::Log("Spawned " + food->GetName() + " at " +
                    std::to_string(x) + "," + std::to_string(y) +
                    " (ID: " + std::to_string(food->GetId()) +
                    ", XP: " + std::to_string(food->GetExperience()) + ")");
            }
            else {
                Logger::Log("ERROR: GetRandomFood returned nullptr!");
            }
        }
        attempts++;
    }

    Logger::Log("Food spawning completed: " + std::to_string(spawned) + "/" +
        std::to_string(count) + " items spawned (" +
        std::to_string(attempts) + " attempts)");
}

/// <summary>
/// Возвращает еду в указанных координатах или nullptr если еды нет
/// </summary>
const Food* World::GetFoodAt(int x, int y) const {
    if (!m_foodManager || x < 0 || x >= m_contentWidth || y < 0 || y >= m_contentHeight) {
        return nullptr;
    }

    int key = y * m_contentWidth + x;
    auto it = m_foodSpawns.find(key);
    if (it != m_foodSpawns.end()) {
        return m_foodManager->GetFood(it->second.foodId);
    }
    return nullptr;
}

/// <summary>
/// Удаляет еду с указанной позиции, когда игрок ее собирает
/// </summary>
bool World::RemoveFoodAt(int x, int y) {
    if (x < 0 || x >= m_contentWidth || y < 0 || y >= m_contentHeight) {
        return false;
    }

    int key = y * m_contentWidth + x;
    auto it = m_foodSpawns.find(key);
    if (it != m_foodSpawns.end()) {
        const Food* food = m_foodManager->GetFood(it->second.foodId);
        if (food) {
            Logger::Log("Food collected: " + food->GetName() + " at " +
                std::to_string(x) + "," + std::to_string(y));
        }
        m_foodSpawns.erase(it);
        return true;
    }
    return false;
}

/// <summary>
/// Респавн еды
/// </summary>
void World::RespawnFoodPeriodically() {
    int currentFoodCount = m_foodSpawns.size();
    int maxFoodOnMap = 40;

    if (currentFoodCount < 40) {
        int foodToSpawn = std::min(10, maxFoodOnMap - currentFoodCount);
        SpawnRandomFood(foodToSpawn);
        Logger::Log("Periodic food respawn: added " + std::to_string(foodToSpawn) + " items");
    }
}

/// <summary>
/// Проверка: можно ли спавнить еду в указанной позиции
/// </summary>
bool World::CanSpawnFoodAt(int x, int y) const {
    if (x < 0 || x >= m_contentWidth || y < 0 || y >= m_contentHeight) {
        return false;
    }

    int key = y * m_contentWidth + x;
    if (m_foodSpawns.find(key) != m_foodSpawns.end()) {
        return false;
    }

    return true;
}

/// <summary>
/// Находит случайную проходимую позицию на карте
/// </summary>
int World::GetRandomPassablePosition(int& outX, int& outY) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> disX(0, m_contentWidth - 1);
    std::uniform_int_distribution<> disY(0, m_contentHeight - 1);

    for (int attempt = 0; attempt < 100; attempt++) {
        int x = disX(gen);
        int y = disY(gen);

        int tileId = GetTileAt(x, y);
        TileType* tile = m_tileManager->GetTileType(tileId);

        if (tile && tile->IsPassable()) {
            outX = x;
            outY = y;
            return tileId;
        }
    }

    // Fallback
    outX = 1;
    outY = 1;
    return GetTileAt(1, 1);
}

/// <summary>
/// Полная очистка еды
/// </summary>
void World::ClearAllFood() {
    int foodCount = m_foodSpawns.size();
    m_foodSpawns.clear();
    Logger::Log("Cleared all food from world: " + std::to_string(foodCount) + " items removed");
}
