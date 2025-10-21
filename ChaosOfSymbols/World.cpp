#include "World.h"
#include "Logger.h"
#include <random>
#include <algorithm>
#include <ctime> 

World::World()
    : m_width(0), m_height(0), m_contentWidth(0), m_contentHeight(0),
    m_automatonEnabled(true), m_tileManager(nullptr), m_foodManager(nullptr),
    m_automatonConfig(nullptr) 
{
}

void World::GenerateFromConfig() {
    Logger::Log("\n=== STARTING PURE RULE-BASED GENERATION ===\n");

    // Загружаем конфигурацию (оба конфига загружаются внутри WorldGenConfig)
    if (!m_config.LoadConfig()) {
        Logger::Log("ERROR: Failed to load world generation config");
        return;
    }

    // Рассчитываем размеры: контент + граница с двух сторон
    m_contentWidth = m_config.GetWidth();
    m_contentHeight = m_config.GetHeight();
    m_width = m_contentWidth + 2;
    m_height = m_contentHeight + 2;

    m_map.resize(m_height, std::vector<int>(m_width, 0));

    // Настраиваем генератор шума
    int currentSeed = m_config.GetEffectiveSeed();
    m_noiseGenerator.SetSeed(currentSeed);
    m_noiseGenerator.SetFrequency(m_config.GetNoiseFrequency());

    Logger::Log("Content size: " + std::to_string(m_contentWidth) + "x" + std::to_string(m_contentHeight));
    Logger::Log("Total size with border: " + std::to_string(m_width) + "x" + std::to_string(m_height));
    Logger::Log("Using seed: " + std::to_string(currentSeed));

    // Загружаем конфиг клеточного автомата
    if (m_automatonConfig) {
        Logger::Log("Cellular automaton config is available (external)");
    }
    else {
        Logger::Log("WARNING: No cellular automaton config available");
    }

    // Генерируем базовый террейн
    GenerateBaseTerrain();

    // Создаем границу из символов #
    CreateBorder();

    SmoothTerrain();

    // Спавним начальную еду
    if (m_foodManager) {
        // Спавним еду в количестве ~10% от площади карты
        int initialFoodCount = (m_contentWidth * m_contentHeight) / 10;
        initialFoodCount = std::min(initialFoodCount, 30); // Но не более 30
        SpawnRandomFood(initialFoodCount);
    }
    else {
        Logger::Log("WARNING: No food manager available for initial food spawn");
    }

    Logger::Log("=== RULE-BASED GENERATION COMPLETED ===");
}

void World::GenerateBaseTerrain() {
    if (!m_tileManager) {
        Logger::Log("ERROR: No tile manager for base terrain generation");
        return;
    }

    const auto& spawnRules = m_config.GetAllSpawnRules();

    if (spawnRules.empty()) {
        Logger::Log("WARNING: No spawn rules found");
        return;
    }

    // Определяем символы динамически из конфига
    char waterChar = FindWaterTile(spawnRules);
    char grassChar = FindGrassTile(spawnRules);
    char mountainChar = FindMountainTile(spawnRules);

    int tilesPlaced = 0;
    std::unordered_map<char, int> tileStatistics;

    for (int y = 1; y < m_height - 1; y++) {
        for (int x = 1; x < m_width - 1; x++) {
            // Более высокая частота шума для меньшего мира
            float baseNoise = (m_noiseGenerator.GetNoise((float)x * 0.03f, (float)y * 0.03f) + 1.0f) * 0.5f;
            float ridgeNoise = 1.0f - std::abs(m_noiseGenerator.GetNoise((float)x * 0.06f + 1000, (float)y * 0.06f + 1000));
            float detailNoise = (m_noiseGenerator.GetNoise((float)x * 0.15f + 2000, (float)y * 0.15f + 2000) + 1.0f) * 0.5f;

            // Комбинируем шумы
            float heightNoise = baseNoise * 0.4f + ridgeNoise * 0.4f + detailNoise * 0.2f;

            // Меньше воды, больше разнообразия
            heightNoise = std::pow(heightNoise, 1.1f); // Меньше эрозии

            // Узкие зоны для большего разнообразия
            int zone;
            if (heightNoise < 0.25f) {  // Только 25% для воды (было 30%)
                zone = 0; // Низкая зона (вода)
            }
            else if (heightNoise < 0.7f) { // 45% для травы
                zone = 1; // Средняя зона (трава)
            }
            else { // 30% для гор
                zone = 2; // Высокая зона (горы)
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

    // Логируем статистику с именами тайлов
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

            // Естественные переходы между биомами
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

char World::SelectTileByZone(int zone, const std::unordered_map<char, SpawnRule>& spawnRules, int x, int y) {
    // Используем координаты для детерминированного, но плавного выбора
    float noise = (m_noiseGenerator.GetNoise((float)x * 0.1f, (float)y * 0.1f) + 1.0f) * 0.5f;

    // Создаем взвешенный выбор на основе вероятностей
    std::vector<std::pair<char, float>> weightedTiles;
    float totalWeight = 0.0f;

    for (const auto& pair : spawnRules) {
        char tileChar = pair.first;
        const SpawnRule& rule = pair.second;

        if (rule.zoneProbabilities.size() > zone) {
            float baseProb = rule.zoneProbabilities[zone];
            // Добавляем небольшую вариативность на основе шума
            float variedProb = baseProb * (0.9f + noise * 0.2f);
            weightedTiles.push_back({ tileChar, variedProb });
            totalWeight += variedProb;
        }
    }

    // Вероятностный выбор вместо выбора самого вероятного
    if (totalWeight > 0.0f) {
        float randomValue = noise * totalWeight; // Используем шум как случайное значение
        float currentWeight = 0.0f;

        for (const auto& weightedTile : weightedTiles) {
            currentWeight += weightedTile.second;
            if (randomValue <= currentWeight) {
                return weightedTile.first;
            }
        }
    }

    // Fallback: возвращаем самый вероятный тайл
    if (!weightedTiles.empty()) {
        std::sort(weightedTiles.begin(), weightedTiles.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });
        return weightedTiles[0].first;
    }

    Logger::Log("No tiles available for zone " + std::to_string(zone) + ", using '.'");
    return '.';
}

void World::CreateBorder() {
    if (!m_tileManager) {
        Logger::Log("ERROR: No tile manager for border creation");
        return;
    }

    // Находим ID для тайла границы (#)
    int borderTileId = FindTileIdByCharacter('#');
    if (borderTileId == -1) {
        Logger::Log("WARNING: Border tile '#' not found, skipping border creation");
        return;
    }

    Logger::Log("Creating border with tile ID: " + std::to_string(borderTileId));

    // Создаем границу по периметру карты
    for (int x = 0; x < m_width; x++) {
        // Верхняя граница (y = 0)
        m_map[0][x] = borderTileId;
        // Нижняя граница (y = m_height - 1)
        m_map[m_height - 1][x] = borderTileId;
    }

    for (int y = 0; y < m_height; y++) {
        // Левая граница (x = 0)
        m_map[y][0] = borderTileId;
        // Правая граница (x = m_width - 1)
        m_map[y][m_width - 1] = borderTileId;
    }

    Logger::Log("Border created successfully");
}

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

            // ИСПРАВЬ ЗДЕСЬ: rule->deathRule(neighborCounts) -> rule->deathRule->evaluate(neighborCounts)
            if (m_map[y][x] != 0 && rule && rule->deathRule) {
                bool shouldDie = rule->deathRule->evaluate(neighborCounts);  // ← ИСПРАВЬ
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

            // ИСПРАВЬ ЗДЕСЬ ВСЕ ВЫЗОВЫ:
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

// Вспомогательный метод для проверки одного соседа
void World::CheckNeighbor(int x, int y, int dx, int dy,
    const std::vector<std::vector<int>>& currentMap,
    std::unordered_map<char, int>& counts) const {
    int nx = x + dx;
    int ny = y + dy;

    // Игнорируем граничные клетки
    if (nx <= 0 || nx >= m_width - 1 || ny <= 0 || ny >= m_height - 1) {
        return;
    }

    if (nx >= 0 && nx < m_width && ny >= 0 && ny < m_height) {
        char neighborChar = GetTileCharacter(currentMap[ny][nx]);
        counts[neighborChar]++;
    }
}

int World::GetTileAt(int x, int y) const {
    // Координаты передаются для игровой области (0,0 - левый верхний угол игрового пространства)
    // Преобразуем в координаты полной карты (с границей)
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

    Logger::Log("Looking for terrain type: " + terrainType);

    // Сначала пытаемся найти по имени в TileType
    const auto& allTiles = m_tileManager->GetAllTiles();
    for (const auto& pair : allTiles) {
        const TileType& tile = pair.second;
        std::string name = tile.GetName();
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);

        if (name.find(terrainType) != std::string::npos) {
            return tile.GetCharacter();
        }
    }

    // Если по имени не нашли, анализируем вероятности из спавн-правил
    for (const auto& spawnPair : spawnRules) {
        char character = spawnPair.first;
        const SpawnRule& rule = spawnPair.second;

        if (rule.zoneProbabilities.size() >= 3) {
            float lowProb = rule.zoneProbabilities[0];  // низины
            float midProb = rule.zoneProbabilities[1];  // равнины
            float highProb = rule.zoneProbabilities[2]; // горы

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

void World::SpawnRandomFood(int count) {
    if (!m_foodManager || !m_tileManager) {
        Logger::Log("WARNING: Cannot spawn food - no food manager or tile manager");
        return;
    }

    Logger::Log("Attempting to spawn " + std::to_string(count) + " food items");

    int spawned = 0;
    int attempts = 0;
    const int maxAttempts = count * 20; // Увеличим количество попыток

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

                if (spawned <= 5) { // Логируем только первые 5 для отладки
                    Logger::Log("Spawned " + food->GetName() + " at " +
                        std::to_string(x) + "," + std::to_string(y));
                }
            }
        }
        attempts++;
    }

    Logger::Log("Food spawning completed: " + std::to_string(spawned) + "/" +
        std::to_string(count) + " items spawned (" +
        std::to_string(attempts) + " attempts)");
}

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

void World::RespawnFoodPeriodically() {
    // Респавним 1-3 единицы еды, если на карте мало еды
    int currentFoodCount = m_foodSpawns.size();
    int maxFoodOnMap = 40; // Максимальное количество еды на карте

    if (currentFoodCount < 40) {
        int foodToSpawn = std::min(10, maxFoodOnMap - currentFoodCount);
        SpawnRandomFood(foodToSpawn);
        Logger::Log("Periodic food respawn: added " + std::to_string(foodToSpawn) + " items");
    }
}

bool World::CanSpawnFoodAt(int x, int y) const {
    // Проверяем, что позиция в пределах игрового пространства
    if (x < 0 || x >= m_contentWidth || y < 0 || y >= m_contentHeight) {
        return false;
    }

    // Проверяем, что здесь уже нет еды
    int key = y * m_contentWidth + x;
    if (m_foodSpawns.find(key) != m_foodSpawns.end()) {
        return false;
    }

    return true;
}

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

void World::ClearAllFood() {
    int foodCount = m_foodSpawns.size();
    m_foodSpawns.clear();
    Logger::Log("Cleared all food from world: " + std::to_string(foodCount) + " items removed");
}

void World::UpdateTileAppearance() {
    if (!m_tileManager) return;

    Logger::Log("Updating tile appearances...");
    int changes = 0;

    // Проходим по всей карте и обновляем отображение
    for (int y = 1; y < m_height - 1; y++) {
        for (int x = 1; x < m_width - 1; x++) {
            int tileId = m_map[y][x];
            TileType* tile = m_tileManager->GetTileType(tileId);

            // Если тайл был удален, заменяем его на дефолтный (трава)
            if (!tile) {
                m_map[y][x] = FindTileIdByCharacter('.');
                changes++;
            }
        }
    }

    if (changes > 0) {
        Logger::Log("Updated " + std::to_string(changes) + " tile appearances");
    }
}

void World::RemoveDeletedTiles(const std::unordered_set<int>& removedTileIds) {
    if (removedTileIds.empty()) return;

    Logger::Log("Removing deleted tiles from world...");
    int replacements = 0;

    for (int y = 1; y < m_height - 1; y++) {
        for (int x = 1; x < m_width - 1; x++) {
            if (removedTileIds.find(m_map[y][x]) != removedTileIds.end()) {
                // Заменяем удаленный тайл на траву
                m_map[y][x] = FindTileIdByCharacter('.');
                replacements++;
            }
        }
    }

    if (replacements > 0) {
        Logger::Log("Replaced " + std::to_string(replacements) + " deleted tiles with grass");
    }
}
