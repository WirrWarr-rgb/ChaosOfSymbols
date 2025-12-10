#include <windows.h>
#include <iostream>
#include <conio.h>
#include <chrono>
#include <thread>
#include <vector>
#include <algorithm> 
#include <ctime>
#include <iomanip>
#include "Game.h"
#include "Logger.h"

static bool rPressed = false;

using namespace std;

/// <summary>
/// Конструктор класса - инициализация всех переменных начальными значениями
/// </summary>
Game::Game()
    : m_isRunning(true), m_currentWorld(nullptr),
    m_renderSystem(nullptr),
    m_playerConfig(nullptr),
    m_playerX(0), m_playerY(0), m_playerSteps(0),
    m_playerHP(0), m_playerHunger(0),
    m_playerXP(0), m_playerLevel(1), m_xpToNextLevel(100),
    m_totalXP(0),
    m_inMainMenu(true),
    m_isPaused(false)
{
    SetDefaultConsoleSize();

    m_mainMenu = std::make_unique<MainMenu>();
    m_pauseMenu = std::make_unique<PauseMenu>();
}

Game::~Game() {
    Shutdown();
}

void Game::InitializeMainMenu() {
    m_mainMenu->Initialize();
    m_inMainMenu = true;
}

/// <summary>
/// Инициализация игры
/// </summary>
/// <returns>Успех/неудачу попытки инициалиоровать игру</returns>
bool Game::Initialize() {
    Logger::Initialize(LogFile);
    Logger::Log("=== GAME INITIALIZATION STARTED ===\n");

    InitializeMainMenu();

    return true;
}

/// <summary>
/// Остановка игры
/// </summary>
void Game::Shutdown() {
    Logger::Log("=== GAME SHUTDOWN STARTED ===");

    if (m_currentWorld) {
        delete m_currentWorld;
        m_currentWorld = nullptr;
    }
    if (m_renderSystem) {
        delete m_renderSystem;
        m_renderSystem = nullptr;
    }

    cout << "\nGame shutdown complete.\n";
    Logger::Log("=== GAME SHUTDOWN COMPLETED ===");
    Logger::Close();
}

/// <summary>
/// Игровой цикл с фикс. временем обновления
/// </summary>
void Game::Run() {
    auto lastTime = chrono::steady_clock::now();

    while (m_isRunning) {
        if (m_shouldReturnToMainMenu) {
            ReturnToMainMenu();
            m_shouldReturnToMainMenu = false;
            continue;
        }
        auto currentTime = chrono::steady_clock::now();
        auto deltaTime = chrono::duration_cast<chrono::milliseconds>(currentTime - lastTime);

        if (deltaTime.count() < FrameDelayMs) {
            this_thread::sleep_for(chrono::milliseconds(FrameDelayMs - deltaTime.count()));
            continue;
        }

        lastTime = currentTime;

        if (m_inMainMenu) {
            RunMainMenu();
        }
        else if (m_isPaused) {
            RunPauseMenu();
        }
        else {
            if (!m_inMainMenu) {
                ProcessInput();
                Update();
                Render();
            }
        }
    }
}

void Game::RunMainMenu() {
    m_mainMenu->ProcessInput();
    m_mainMenu->Update();
    m_mainMenu->Render();

    if (m_mainMenu->ShouldStartGame()) {
        if (m_mainMenu->ShouldLoadSave()) {
            StartGameFromSave(m_mainMenu->GetSelectedSaveGameMode(), m_mainMenu->GetSelectedSaveSlot());
        }
        else {
            Logger::Log("Starting NEW game from menu selection...");
            StartGameFromMenu();
        }
        m_mainMenu->ResetStartFlags();
    }
    else if (m_mainMenu->ShouldExitGame()) {
        Logger::Log("Exit requested from main menu");
        m_isRunning = false;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(16));
}

void Game::StartGameFromSave(GameMode mode, int slot) {
    Logger::Log("Starting game from save: mode=" +
        std::to_string(static_cast<int>(mode)) +
        ", slot=" + std::to_string(slot));

    if (!m_saveSystem) {
        m_saveSystem = std::make_unique<SaveSystem>();
    }

    if (!m_saveSystem->LoadSave(mode, slot)) {
        Logger::Log("ERROR: Failed to load save slot " + std::to_string(slot));
        StartGameFromMenu();
        return;
    }

    const WorldConfig& loadedConfig = m_saveSystem->GetLoadedConfig();

    LoadWorldFromSave(loadedConfig);

    m_inMainMenu = false;
    m_renderSystem->ClearScreen();

    Logger::Log("=== GAME STARTED FROM SAVE ===");
}

void Game::LoadWorldFromSave(const WorldConfig& config) {
    Logger::Log("Loading world from save configuration...");
    Logger::Log("Starting game from save at: " + Logger::GetTickCount());

    m_configManager = std::make_unique<ConfigManager>();
    if (!m_configManager->Initialize()) {
        Logger::Log("ERROR: Failed to initialize config manager!");
        return;
    }

    std::string saveSlot = std::to_string(m_mainMenu->GetSelectedSaveSlot());
    std::string tilesPath = "saves/proceduralGeneration/slot" + saveSlot + "/tiles.json";
    std::string foodPath = "saves/proceduralGeneration/slot" + saveSlot + "/food.cfg";


    TileTypeManager* tileManager = m_configManager->GetTileManager();
    FoodManager* foodManager = m_configManager->GetFoodManager();

    tileManager->SetFilePath(tilesPath);
    if (!tileManager->LoadFromFile()) {
        Logger::Log("WARNING: Could not load tiles from save slot");
        // Пробуем загрузить дефолтный
        tileManager->SetFilePath("config/tiles.json");
        if (!tileManager->LoadFromFile()) {
            Logger::Log("ERROR: Failed to load default tiles config");
        }
    }

    // ВАЖНО: Загружаем конфигурацию еды из сейва
    if (!foodManager->LoadFromFile(foodPath)) {
        Logger::Log("ERROR: Could not load food config from save slot!");
        // Попробуем загрузить дефолтную конфигурацию
        if (!foodManager->LoadFromFile("config/default_food.cfg")) {
            Logger::Log("ERROR: Failed to load default food config");
        }
    }

    Logger::Log("Loaded " + std::to_string(tileManager->GetAllTiles().size()) +
        " tiles and " + std::to_string(foodManager->GetAllFood().size()) +
        " food items from slot " + saveSlot);

    m_playerConfig = m_configManager->GetPlayerConfig();

    m_playerConfig->SetMaxHP(config.GetPlayerMaxHP());
    m_playerConfig->SetMaxHunger(config.GetPlayerMaxHunger());
    m_playerConfig->SetHPEnabled(config.GetEnableHP());
    m_playerConfig->SetHungerEnabled(config.GetEnableHunger());

    m_playerX = config.GetPlayerStartX();
    m_playerY = config.GetPlayerStartY();

    int worldWidth = config.GetWidth();
    int worldHeight = config.GetHeight();

    if (m_playerX >= worldWidth) m_playerX = worldWidth / 2;
    if (m_playerY >= worldHeight) m_playerY = worldHeight / 2;
    if (m_playerX < 0) m_playerX = 1;
    if (m_playerY < 0) m_playerY = 1;

    m_playerHP = config.GetPlayerMaxHP();
    m_playerHunger = config.GetPlayerMaxHunger();
    m_xpToNextLevel = 100;

    // УДАЛИТЬ ЭТУ СТРОКУ - foodManager уже объявлен выше!
    // FoodManager* foodManager = m_configManager->GetFoodManager();  // <-- ЭТО ДУБЛИРОВАНИЕ

    m_currentWorld = new World();

    m_currentWorld->SetTileManager(tileManager);
    m_currentWorld->SetFoodManager(foodManager);  // Используем уже объявленный foodManager
    m_currentWorld->SetAutomatonEnabled(true);
    m_currentWorld->SetAutomatonConfig(m_configManager->GetAutomatonConfig());

    WorldConfig* worldConfig = m_currentWorld->GetWorldConfig();
    if (worldConfig) {
        worldConfig->SetWidth(worldWidth);
        worldConfig->SetHeight(worldHeight);
        worldConfig->SetSeed(config.GetSeed());
        worldConfig->SetNoiseFrequency(config.GetNoiseFrequency());
        worldConfig->SetNeighborRadius(config.GetNeighborRadius());

        if (config.GetRandomGeneration()) {
            worldConfig->SetGenerationMode(WorldGenerationMode::RANDOM);
        }
        else {
            worldConfig->SetGenerationMode(WorldGenerationMode::SEEDED);
        }

        worldConfig->SetMapFilePath("");

        worldConfig->MarkAsLoadedFromSave();

        auto& spawnRules = const_cast<std::unordered_map<char, SpawnRule>&>(worldConfig->GetAllSpawnRules());
        spawnRules.clear();

        for (const auto& pair : config.GetTileProbabilities()) {
            SpawnRule rule;
            rule.character = pair.first;
            rule.zoneProbabilities = pair.second;
            spawnRules[pair.first] = rule;
        }

        Logger::Log("World config set from save: " +
            std::to_string(worldWidth) + "x" + std::to_string(worldHeight) +
            ", seed: " + std::to_string(config.GetSeed()) +
            ", mode: " + (config.GetRandomGeneration() ? "RANDOM" : "SEEDED"));
    }

    Logger::Log("Generating world from save configuration...");
    m_currentWorld->GenerateFromConfig();

    m_renderSystem = new RenderSystem(tileManager);

    int totalWorldWidth = m_currentWorld->GetTotalWidth();
    int totalWorldHeight = m_currentWorld->GetTotalHeight();
    m_renderSystem->SetScreenSize(totalWorldWidth, totalWorldHeight);

    EnsureValidPlayerPosition();

    m_configManager->OnTilesChanged = [this]() { this->OnTilesChanged(); };
    m_configManager->OnFoodChanged = [this]() { this->OnFoodChanged(); };
    m_configManager->OnAutomatonRulesChanged = [this]() { this->OnAutomatonRulesChanged(); };

    Logger::Log("World loaded successfully from save");
    Logger::Log("World size: " + std::to_string(worldWidth) + "x" + std::to_string(worldHeight));
    Logger::Log("Render system size: " + std::to_string(totalWorldWidth) + "x" + std::to_string(totalWorldHeight));
    Logger::Log("Player position: " + std::to_string(m_playerX) + "," + std::to_string(m_playerY));
}

void Game::StartGameFromMenu() {
    Logger::Log("Starting game from main menu...");

    SetDefaultConsoleSize();

    m_configManager = std::make_unique<ConfigManager>();
    if (!m_configManager->Initialize()) {
        Logger::Log("ERROR: Failed to initialize config manager!");
        return;
    }

    m_playerConfig = m_configManager->GetPlayerConfig();
    m_playerX = m_playerConfig->GetDefaultPlayerX();
    m_playerY = m_playerConfig->GetDefaultPlayerY();
    m_playerHP = m_playerConfig->GetMaxHP();
    m_playerHunger = m_playerConfig->GetMaxHunger();
    m_xpToNextLevel = m_playerConfig->GetBaseXP();

    m_configManager->OnTilesChanged = [this]() { this->OnTilesChanged(); };
    m_configManager->OnFoodChanged = [this]() { this->OnFoodChanged(); };
    m_configManager->OnAutomatonRulesChanged = [this]() { this->OnAutomatonRulesChanged(); };

    TileTypeManager* tileManager = m_configManager->GetTileManager();
    FoodManager* foodManager = m_configManager->GetFoodManager();

    m_currentWorld = new World();
    m_currentWorld->SetTileManager(tileManager);
    m_currentWorld->SetFoodManager(foodManager);
    m_currentWorld->SetAutomatonEnabled(true);
    m_currentWorld->SetAutomatonConfig(m_configManager->GetAutomatonConfig());

    m_renderSystem = new RenderSystem(tileManager);

    auto gameMode = m_mainMenu->GetSelectedGameMode();
    if (gameMode == MainMenuOption::PLAY_PROCEDURAL) {
        m_currentWorld->GenerateFromConfig();
        Logger::Log("Started game with procedural generation");
    }
    else {
        m_currentWorld->GenerateFromConfig();
        Logger::Log("Started game with preloaded map (placeholder)");
    }

    m_renderSystem->SetScreenSize(m_currentWorld->GetTotalWidth(), m_currentWorld->GetTotalHeight());

    if (m_playerX >= m_currentWorld->GetWidth() || m_playerY >= m_currentWorld->GetHeight()) {
        m_playerX = (m_currentWorld->GetWidth() / 2 > 1) ? m_currentWorld->GetWidth() / 2 : 1;
        m_playerY = (m_currentWorld->GetHeight() / 2 > 1) ? m_currentWorld->GetHeight() / 2 : 1;
    }
    EnsureValidPlayerPosition();

    m_inMainMenu = false;
    m_renderSystem->ClearScreen();

    Logger::Log("=== GAME STARTED FROM MAIN MENU ===");
}

/// <summary>
/// Обработка ввода
/// </summary>
void Game::ProcessInput() {
    if (!m_isRunning || m_shouldReturnToMainMenu) return;

    HandlePauseInput();

    if (m_isPaused) {
        return;
    }

    if (m_playerHP <= 0) {
        ShowDeathScreen();
        return;
    }

    static auto lastMoveTime = chrono::steady_clock::now();
    auto currentTime = chrono::steady_clock::now();
    auto elapsed = chrono::duration_cast<chrono::milliseconds>(currentTime - lastMoveTime);

    if (elapsed.count() < m_playerConfig->GetMoveCooldownMs()) {
        return;
    }

    bool playerMoved = false;

    if (GetAsyncKeyState('W') & 0x8000 || GetAsyncKeyState(VK_UP) & 0x8000) {
        MovePlayer(0, -1);
        lastMoveTime = currentTime;
        playerMoved = true;
    }
    if (GetAsyncKeyState('S') & 0x8000 || GetAsyncKeyState(VK_DOWN) & 0x8000) {
        MovePlayer(0, 1);
        lastMoveTime = currentTime;
        playerMoved = true;
    }
    if (GetAsyncKeyState('A') & 0x8000 || GetAsyncKeyState(VK_LEFT) & 0x8000) {
        MovePlayer(-1, 0);
        lastMoveTime = currentTime;
        playerMoved = true;
    }
    if (GetAsyncKeyState('D') & 0x8000 || GetAsyncKeyState(VK_RIGHT) & 0x8000) {
        MovePlayer(1, 0);
        lastMoveTime = currentTime;
        playerMoved = true;
    }

    if (playerMoved && m_currentWorld->IsAutomatonEnabled()) {
        static int automatonCounter = 0;
        if (++automatonCounter >= 1) {
            Logger::Log("Player moved - updating cellular automaton");
            m_currentWorld->UpdateCellularAutomaton();
            automatonCounter = 0;
        }
        m_playerSteps++;
        ConsumeEnergy();
    }

    if (GetAsyncKeyState('Q') & 0x8000) {
        m_isRunning = false;
    }

    if (GetAsyncKeyState('R') & 0x8000) {
        if (!rPressed) {
            Logger::Log("Regenerating world from config...");

            m_currentWorld->ClearAllFood();
            m_currentWorld->GenerateFromConfig();

            m_playerSteps = 0;
            m_playerHP = m_playerConfig->GetMaxHP();
            m_playerHunger = m_playerConfig->GetMaxHunger();
            m_totalXP = 0;

            m_foodEaten.clear();

            m_renderSystem->SetScreenSize(m_currentWorld->GetTotalWidth(), m_currentWorld->GetTotalHeight());

            EnsureValidPlayerPosition();
            rPressed = true;
        }
    }
    else {
        rPressed = false;
    }
}

/// <summary>
/// Система голода и здоровья
/// </summary>
void Game::ConsumeEnergy() {
    if (m_playerHunger > 0) {
        m_playerHunger--;
        Logger::Log("Hunger decreased: " + std::to_string(m_playerHunger) + "/" +
            std::to_string(m_playerConfig->GetMaxHunger()));
    }

    if (m_playerHunger <= 0 && m_playerConfig->IsHPEnabled()) {
        m_playerHP -= 2;
        Logger::Log("Starving! HP decreased: " + std::to_string(m_playerHP) + "/" +
            std::to_string(m_playerConfig->GetMaxHP()));

        if (m_playerHP <= 0) {
            m_playerHP = 0;
            Logger::Log("Player died from starvation!");
            ShowDeathScreen();
        }
    }
}

/// <summary>
/// Обновление состояния
/// </summary>
void Game::Update() {
    if (!m_isRunning) return;

    if (m_configManager) {
        m_configManager->Update();
    }

    if (m_playerHP <= 0) {
        ShowDeathScreen();
        return;
    }

    CollectFood();

    static int foodRespawnTimer = 0;
    if (++foodRespawnTimer > 300) {
        m_currentWorld->RespawnFoodPeriodically();
        foodRespawnTimer = 0;
    }

    TileTypeManager* tileManager = m_configManager->GetTileManager();
    int currentTile = m_currentWorld->GetTileAt(m_playerX, m_playerY);
    TileType* tile = tileManager->GetTileType(currentTile);

    if (tile && !tile->IsPassable()) {
        FindNearestPassablePosition();
    }
}

/// <summary>
/// Сбор еды и опыта
/// </summary>
void Game::CollectFood() {
    const Food* food = m_currentWorld->GetFoodAt(m_playerX, m_playerY);
    if (food) {
        int foodId = food->GetId();
        if (m_foodEaten.find(foodId) == m_foodEaten.end()) {
            m_foodEaten[foodId] = 0;
        }

        m_foodEaten[foodId]++;

        int oldHP = m_playerHP;
        int oldHunger = m_playerHunger;

        m_playerHunger = min(m_playerConfig->GetMaxHunger(), m_playerHunger + food->GetHungerRestore());
        m_playerHP = min(m_playerConfig->GetMaxHP(), m_playerHP + food->GetHpRestore());

        int xpGained = food->GetExperience();
        GainXP(xpGained);

        m_currentWorld->RemoveFoodAt(m_playerX, m_playerY);

        Logger::Log("Collected " + food->GetName() +
            " - Hunger: +" + std::to_string(food->GetHungerRestore()) +
            ", HP: +" + std::to_string(food->GetHpRestore()) +
            ", XP: +" + std::to_string(xpGained) +
            " | Now: HP=" + std::to_string(m_playerHP) +
            "/" + std::to_string(m_playerConfig->GetMaxHP()) +
            ", Hunger=" + std::to_string(m_playerHunger) +
            "/" + std::to_string(m_playerConfig->GetMaxHunger()) +
            ", XP=" + std::to_string(m_playerXP) +
            "/" + std::to_string(m_xpToNextLevel) +
            ", Level=" + std::to_string(m_playerLevel));
    }
}

/// <summary>
/// Отрисовка
/// </summary>
void Game::Render() {
    if (m_isPaused) {
        rlutil::hideCursor();
        m_pauseMenu->Render();
        return;
    }

    if (!m_renderSystem) {
        return;
    }

    rlutil::hideCursor();

    static int lastPlayerX = m_playerX;
    static int lastPlayerY = m_playerY;

    m_renderSystem->StartFrame();

    m_renderSystem->DrawWorld(*m_currentWorld);
    m_renderSystem->DrawPlayer(m_playerX, m_playerY, lastPlayerX, lastPlayerY, *m_currentWorld);

    static int uiCounter = 0;
    if (uiCounter++ > UiUpdateInterval) {
        m_renderSystem->DrawUI(*m_currentWorld, m_playerX, m_playerY, m_playerSteps,
            m_playerHP, m_playerConfig->GetMaxHP(),
            m_playerHunger, m_playerConfig->GetMaxHunger(),
            m_playerXP, m_playerLevel, m_xpToNextLevel);
        uiCounter = 0;
    }

    m_renderSystem->EndFrame();

    lastPlayerX = m_playerX;
    lastPlayerY = m_playerY;
}


/// <summary>
/// Движение и позиционирование игрока
/// </summary>
/// <param name="dx">Смещение по X (-1 : влево, 0 : на месте, 1 : вправо</param>
/// <param name="dy">Смещение по Y (-1 : вверх, 0 : на месте, 1 : вниз</param>
bool Game::MovePlayer(int dx, int dy) {
    int newX = m_playerX + dx;
    int newY = m_playerY + dy;

    TileTypeManager* tileManager = m_configManager->GetTileManager();

    if (newX >= 0 && newX < m_currentWorld->GetWidth() &&
        newY >= 0 && newY < m_currentWorld->GetHeight()) {

        int targetTile = m_currentWorld->GetTileAt(newX, newY);
        TileType* tile = tileManager->GetTileType(targetTile);

        const Food* food = m_currentWorld->GetFoodAt(newX, newY);
        if (food) {
            m_playerX = newX;
            m_playerY = newY;
            return true;
        }

        if (tile && tile->IsPassable()) {
            m_playerX = newX;
            m_playerY = newY;
            return true;
        }
    }
    return false;
}

/// <summary>
/// Проверяет текущую позицию и исправляет ее при необходимости
/// </summary>
void Game::EnsureValidPlayerPosition() {
    TileTypeManager* tileManager = m_configManager->GetTileManager();

    int currentTile = m_currentWorld->GetTileAt(m_playerX, m_playerY);
    TileType* tile = tileManager->GetTileType(currentTile);

    if (!tile || !tile->IsPassable()) {
        FindNearestPassablePosition();
    }
}

/// <summary>
/// Поиск ближайшего проходимого места
/// </summary>
void Game::FindNearestPassablePosition() {
    TileTypeManager* tileManager = m_configManager->GetTileManager();

    for (int radius = 1; radius <= MaxSearchRadius; radius++) {
        for (int dy = -radius; dy <= radius; dy++) {
            for (int dx = -radius; dx <= radius; dx++) {
                if (radius > 1 && abs(dx) < radius && abs(dy) < radius) {
                    continue;
                }

                int checkX = m_playerX + dx;
                int checkY = m_playerY + dy;

                if (checkX >= 0 && checkX < m_currentWorld->GetWidth() &&
                    checkY >= 0 && checkY < m_currentWorld->GetHeight()) {

                    int tileId = m_currentWorld->GetTileAt(checkX, checkY);
                    TileType* tile = tileManager->GetTileType(tileId);

                    if (tile && tile->IsPassable()) {
                        m_playerX = checkX;
                        m_playerY = checkY;
                        return;
                    }
                }
            }
        }
    }

    FindRandomPassablePosition();
}

/// <summary>
/// Поиск рандомного проходимого места
/// </summary>
void Game::FindRandomPassablePosition() {
    TileTypeManager* tileManager = m_configManager->GetTileManager();

    cout << "Using fallback: searching for random passable position..." << endl;

    srand(static_cast<unsigned int>(time(nullptr)));

    for (int attempt = 0; attempt < MaxRandomAttempts; attempt++) {
        int randomX = rand() % m_currentWorld->GetWidth();
        int randomY = rand() % m_currentWorld->GetHeight();

        int tileId = m_currentWorld->GetTileAt(randomX, randomY);
        TileType* tile = tileManager->GetTileType(tileId);

        if (tile && tile->IsPassable()) {
            m_playerX = randomX;
            m_playerY = randomY;
            return;
        }
    }

    m_playerX = EmergencyPositionX;
    m_playerY = EmergencyPositionY;
}

/// <summary>
/// Экран смерти
/// </summary>
void Game::ShowDeathScreen() {
    int finalSteps = m_playerSteps;
    int finalTotalXP = m_totalXP;
    int finalLevel = m_playerLevel;
    int finalXP = m_playerXP;
    auto finalFoodEaten = m_foodEaten;

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    for (int i = 0; i < 5; i++) {
        system("cls");
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);

    COORD topLeft = { 0, 0 };
    DWORD consoleSize = csbi.dwSize.X * csbi.dwSize.Y;
    DWORD written;

    FillConsoleOutputCharacterA(hConsole, ' ', consoleSize, topLeft, &written);
    FillConsoleOutputAttribute(hConsole, 7, consoleSize, topLeft, &written);
    SetConsoleCursorPosition(hConsole, topLeft);

    SetConsoleTextAttribute(hConsole, 7);
    cout << "==========================================\n";

    SetConsoleTextAttribute(hConsole, 12);
    cout << "\n           YOU DIED! Unlucky :)\n";

    SetConsoleTextAttribute(hConsole, 7);
    cout << "\n==========================================\n";
    cout << "\nSteps: " << finalSteps << "\n";
    cout << "Total XP: " << finalTotalXP << "\n";
    cout << "Current Level: " << finalLevel << "\n";
    cout << "Current XP: " << finalXP << "\n";
    cout << "\n==========================================\n";
    cout << "\nFood consumed:\n";

    FoodManager* foodManager = m_configManager->GetFoodManager();

    const auto& allFood = foodManager->GetAllFood();

    for (const auto& foodType : allFood) {
        int foodId = foodType->GetId();

        int count = 0;
        auto it = finalFoodEaten.find(foodId);
        if (it != finalFoodEaten.end()) {
            count = it->second;
        }

        SetConsoleTextAttribute(hConsole, foodType->GetColor());

        cout << foodType->GetName() << ": " << count;

        SetConsoleTextAttribute(hConsole, 7);
        cout << "\n";
    }

    SetConsoleTextAttribute(hConsole, 7);
    cout << "\n==========================================\n";
    SetConsoleTextAttribute(hConsole, 7);
    cout << "\nPress ESC to return to Main Menu..." << "\n";

    while (true) {
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    SetConsoleTextAttribute(hConsole, 7);

    ClearScreenCompletely();

    m_isRunning = false;
}

void Game::ClearScreenCompletely() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);

    DWORD consoleSize = csbi.dwSize.X * csbi.dwSize.Y;
    COORD topLeft = { 0, 0 };
    DWORD written;

    FillConsoleOutputCharacterA(hConsole, ' ', consoleSize, topLeft, &written);

    FillConsoleOutputAttribute(hConsole, csbi.wAttributes, consoleSize, topLeft, &written);

    SetConsoleCursorPosition(hConsole, topLeft);
}


/// <summary>
/// Добавление опыта игроку; проверка возможность lvl up
/// </summary>
/// <param name="amount"></param>
void Game::GainXP(int amount) {
    m_playerXP += amount;
    m_totalXP += amount;

    Logger::Log("Gained " + std::to_string(amount) + " XP! Total: " +
        std::to_string(m_playerXP) + "/" + std::to_string(m_xpToNextLevel) +
        ", Lifetime: " + std::to_string(m_totalXP));

    CheckLevelUp();
}

/// <summary>
/// Проверка и обработка повышения уровня игрока, когда накоплено достаточно опыта
/// </summary>
void Game::CheckLevelUp() {
    while (m_playerXP >= m_xpToNextLevel) {
        m_playerXP -= m_xpToNextLevel;
        m_playerLevel++;

        m_xpToNextLevel = static_cast<int>(m_xpToNextLevel * m_playerConfig->GetXPMultiplier());

        if (m_playerConfig->IsHPEnabled()) {
            m_playerHP = m_playerConfig->GetMaxHP();
        }
        if (m_playerConfig->IsHungerEnabled()) {
            m_playerHunger = m_playerConfig->GetMaxHunger();
        }

        Logger::Log("LEVEL UP! Reached level " + std::to_string(m_playerLevel) +
            "! Next level at " + std::to_string(m_xpToNextLevel) + " XP");
    }
}

/// <summary>
/// Обработка изменений конфигураций тайлов в реальном времени
/// </summary>
void Game::OnTilesChanged() {
    Logger::Log("Tile configurations changed - updating world...");

    if (!m_currentWorld || !m_configManager->GetTileManager()) return;

    m_currentWorld->UpdateTileAppearance();

    EnsureValidPlayerPosition();

    if (m_renderSystem) {
        m_renderSystem->ClearScreen();
    }

    Logger::Log("Tile changes applied successfully");
}

/// <summary>
/// Обработка изменений конфигураций еды в реальном времени
/// </summary>
void Game::OnFoodChanged() {
    Logger::Log("Food configurations changed - updating food...");

    if (m_currentWorld) {
        m_currentWorld->ClearAllFood();

        int initialFoodCount = (m_currentWorld->GetWidth() * m_currentWorld->GetHeight()) / 10;
        initialFoodCount = std::min<int>(initialFoodCount, 30);
        m_currentWorld->SpawnRandomFood(initialFoodCount);

        Logger::Log("Food updated with new configurations");
    }
}

/// <summary>
/// Обработка изменений конфигураций клеточного автомата в реальном времени
/// </summary>
void Game::OnAutomatonRulesChanged() {
    Logger::Log("Automaton rules changed - updating rules...");

    if (m_currentWorld && m_configManager->GetAutomatonConfig()) {
        m_currentWorld->SetAutomatonConfig(m_configManager->GetAutomatonConfig());

        m_currentWorld->UpdateCellularAutomaton();

        EnsureValidPlayerPosition();

        Logger::Log("New automaton rules applied successfully");
    }
}

void Game::HandlePauseInput() {
    static bool escapePressed = false;

    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
        if (!escapePressed) {
            m_isPaused = !m_isPaused;
            if (m_isPaused) {
                m_pauseMenu->Initialize();
                m_pauseMenu->Reset();
                Logger::Log("Game paused");
            }
            else {
                Logger::Log("Game resumed");
            }
            escapePressed = true;
        }
    }
    else {
        escapePressed = false;
    }
}

void Game::RunPauseMenu() {
    m_pauseMenu->ProcessInput();
    m_pauseMenu->Update();
    m_pauseMenu->Render();

    if (m_pauseMenu->ShouldResume()) {
        m_isPaused = false;
        m_pauseMenu->Reset();

        if (m_renderSystem) {
            m_renderSystem->ClearScreen();
        }
        Logger::Log("Game resumed from pause menu");
    }
    else if (m_pauseMenu->ShouldReturnToMainMenu()) {
        Logger::Log("Pause menu requested return to main menu");
        ReturnToMainMenu();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
}

void Game::ReturnToMainMenu() {
    Logger::Log("Returning to main menu...");

    m_isPaused = false;
    m_inMainMenu = true;

    m_pauseMenu->Reset();
    m_mainMenu->Reset();
    m_mainMenu->ResetStartFlags();

    if (m_currentWorld) {
        Logger::Log("Deleting world...");
        delete m_currentWorld;
        m_currentWorld = nullptr;
    }

    if (m_renderSystem) {
        Logger::Log("Deleting render system...");
        delete m_renderSystem;
        m_renderSystem = nullptr;
    }

    if (m_configManager) {
        Logger::Log("Resetting config manager...");
        m_configManager.reset();
    }

    m_playerX = 0;
    m_playerY = 0;
    m_playerSteps = 0;
    m_playerHP = 0;
    m_playerHunger = 0;
    m_playerXP = 0;
    m_playerLevel = 1;
    m_xpToNextLevel = 100;
    m_totalXP = 0;
    m_foodEaten.clear();

    SetDefaultConsoleSize();
    rlutil::cls();

    Logger::Log("Successfully returned to main menu");
}

void Game::SetDefaultConsoleSize() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    COORD bufferSize = { 80, 25 };
    SetConsoleScreenBufferSize(hConsole, bufferSize);

    SMALL_RECT windowSize = { 0, 0, 79, 24 };
    SetConsoleWindowInfo(hConsole, TRUE, &windowSize);

    Logger::Log("Default console size set to 80x25");
}