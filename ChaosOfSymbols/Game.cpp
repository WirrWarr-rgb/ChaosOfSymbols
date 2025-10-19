#include "Game.h"
#include "Logger.h"
#include <windows.h>
#include <iostream>
#include <conio.h>
#include <chrono>
#include <thread>
#include <vector>
#include <algorithm>
#include <ctime>
#include <iomanip>

static bool rPressed = false;

using namespace std;

Game::Game()
    : m_isRunning(false), m_currentWorld(nullptr),
    m_settings(nullptr), m_renderSystem(nullptr),
    m_tileManager(nullptr), m_foodManager(nullptr),
    m_playerX(DefaultPlayerX), m_playerY(DefaultPlayerY),
    m_playerSteps(0),
    m_playerHP(MAX_HP),
    m_playerHunger(MAX_HUNGER),
    m_playerXP(0),           // Добавьте это
    m_playerLevel(1),        // Добавьте это  
    m_xpToNextLevel(100)     // Добавьте это
{
}

Game::~Game() {
    Shutdown();
}

/// <summary>
/// Инициализация игры
/// </summary>
/// <returns>Успех/неудачу попытки инициалиоровать игру</returns>
bool Game::Initialize() {
    Logger::Initialize(LogFile);
    Logger::Log("=== GAME INITIALIZATION STARTED ===");
    Logger::Log("DEBUG Initialize - XP: " + std::to_string(m_playerXP) +
        ", Level: " + std::to_string(m_playerLevel) +
        ", XP to next: " + std::to_string(m_xpToNextLevel));

    m_settings = new GameSettings();
    m_settings->LoadFromFile();

    m_tileManager = new TileTypeManager();

    Logger::Log("Attempting to load tile types...");
    if (!m_tileManager->LoadFromFile()) {
        Logger::Log("ERROR: Failed to load tile types!");
        return false;
    }
    else {
        Logger::Log("Tile types loaded successfully");

        TileType* mountainTile = m_tileManager->GetTileType(4);
        if (mountainTile) {
            char mountainChar = mountainTile->GetCharacter();
            Logger::Log("MOUNTAIN TILE TEST - Character: '" + std::string(1, mountainChar) + "'");
            Logger::Log("MOUNTAIN TILE TEST - Character code: " + std::to_string(static_cast<unsigned char>(mountainChar)));
        }

        int tileCount = m_tileManager->GetTileCount();
        Logger::Log("Number of loaded tiles: " + to_string(tileCount));

        for (int i = 0; i < tileCount; i++) {
            TileType* tile = m_tileManager->GetTileType(i);
            if (tile) {
                Logger::Log("Tile " + to_string(i) + ": " + tile->GetName() + " ('" + string(1, tile->GetCharacter()) + "')");
            }
            else {
                Logger::Log("WARNING: Tile " + to_string(i) + " not found!");
            }
        }
    }

    m_foodManager = new FoodManager();
    Logger::Log("Attempting to load food types...");
    if (!m_foodManager->LoadFromFile()) {
        Logger::Log("WARNING: Failed to load food types!");
        // Не прерываем игру, если не удалось загрузить еду
    }
    else {
        Logger::Log("Food types loaded successfully");
    }


    Logger::Log("Testing config-based generation...");

    m_currentWorld = new World();
    m_currentWorld->SetTileManager(m_tileManager);

    m_currentWorld->SetFoodManager(m_foodManager);

    // Включаем клеточный автомат
    m_currentWorld->SetAutomatonEnabled(true);

    m_renderSystem = new RenderSystem(m_tileManager);

    m_currentWorld->GenerateFromConfig(WorldConfigFile);

    // Устанавливаем размер экрана по игровому пространству (без границы)
    m_renderSystem->SetScreenSize(m_currentWorld->GetTotalWidth(), m_currentWorld->GetTotalHeight());

    // Позиция игрока в игровом пространстве (без учета границы)
    m_playerX = (m_currentWorld->GetWidth() / 2 > 1) ? m_currentWorld->GetWidth() / 2 : 1;
    m_playerY = (m_currentWorld->GetHeight() / 2 > 1) ? m_currentWorld->GetHeight() / 2 : 1;
    EnsureValidPlayerPosition();

    m_isRunning = true;
    Logger::Log("=== GAME INITIALIZATION COMPLETED ===");

    m_renderSystem->ClearScreen();

    return true;
}

void Game::Shutdown() {
    Logger::Log("=== GAME SHUTDOWN STARTED ===");

    cout << "Shutting down game..." << endl;

    if (m_settings) {
        m_settings->SaveToFile();
        delete m_settings;
    }

    delete m_currentWorld;
    delete m_tileManager;
    delete m_renderSystem;

    cout << "Game shutdown complete." << endl;
    Logger::Log("=== GAME SHUTDOWN COMPLETED ===");

    Logger::Close();
}

/// <summary>
/// Игровой цикл
/// </summary>
void Game::Run() {
    auto lastTime = chrono::steady_clock::now(); //начальной время для расчеты дельта времени

    while (m_isRunning) {
        auto currentTime = chrono::steady_clock::now(); //тек. время
        auto deltaTime = chrono::duration_cast<chrono::milliseconds>(currentTime - lastTime); //разница с предыдущим кадром

        if (deltaTime.count() < FrameDelayMs) { //если прошло меньше N мс, то ждем оставшееся время и переходим к следующей итерации
            this_thread::sleep_for(chrono::milliseconds(FrameDelayMs - deltaTime.count()));
            continue;
        }

        lastTime = currentTime;

        ProcessInput();
        Update();
        Render();
    }
}

/// <summary>
/// Обработка ввода
/// </summary>
void Game::ProcessInput() {
    if (!m_isRunning) return;

    if (m_playerHP <= 0) {
        ShowDeathScreen();
        return; // Важно: выходим сразу после показа экрана смерти
    }

    static auto lastMoveTime = chrono::steady_clock::now();
    auto currentTime = chrono::steady_clock::now();
    auto elapsed = chrono::duration_cast<chrono::milliseconds>(currentTime - lastMoveTime);

    if (elapsed.count() < MoveCooldownMs) {
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

    // Обновляем клеточный автомат только если игрок действительно двигался
    if (playerMoved && m_currentWorld->IsAutomatonEnabled()) {
        static int automatonCounter = 0;
        if (++automatonCounter >= 3) {
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

            // ОЧИСТКА ЕДЫ ПЕРЕД РЕГЕНЕРАЦИЕЙ - ДОБАВЬТЕ ЭТО
            m_currentWorld->ClearAllFood();

            m_currentWorld->GenerateFromConfig("config/world_gen.cfg");

            // СБРОС ПОКАЗАТЕЛЕЙ
            m_playerSteps = 0;
            m_playerHP = MAX_HP;
            m_playerHunger = MAX_HUNGER;

            // Обновляем размер экрана по полному размеру (с границей)
            m_renderSystem->SetScreenSize(m_currentWorld->GetTotalWidth(), m_currentWorld->GetTotalHeight());

            EnsureValidPlayerPosition();
            rPressed = true;
        }
    }
    else {
        rPressed = false;
    }
}

void Game::ConsumeEnergy() {
    // Уменьшаем голод на 1
    if (m_playerHunger > 0) {
        m_playerHunger--;
        Logger::Log("Hunger decreased: " + std::to_string(m_playerHunger) + "/" + std::to_string(MAX_HUNGER));
    }

    // Если голод 0, отнимаем HP
    if (m_playerHunger <= 0) {
        m_playerHP -= 2;
        Logger::Log("Starving! HP decreased: " + std::to_string(m_playerHP) + "/" + std::to_string(MAX_HP));

        // Проверяем смерть
        if (m_playerHP <= 0) {
            m_playerHP = 0;
            Logger::Log("Player died from starvation!");

            // ОСТАНАВЛИВАЕМ ИГРУ И ПОКАЗЫВАЕМ ЭКРАН СМЕРТИ
            m_isRunning = false;
            ShowDeathScreen();
        }
    }
}

/// <summary>
/// Обновление состояния
/// </summary>
void Game::Update() {
    if (!m_isRunning) return;

    if (m_playerHP <= 0) {
        ShowDeathScreen();
        return;
    }

    CollectFood();

    static int foodRespawnTimer = 0;
    if (++foodRespawnTimer > 300) { // Каждые 300 обновлений
        m_currentWorld->RespawnFoodPeriodically();
        foodRespawnTimer = 0;
    }

    int currentTile = m_currentWorld->GetTileAt(m_playerX, m_playerY);
    TileType* tile = m_tileManager->GetTileType(currentTile);

    if (tile && !tile->IsPassable()) {
        FindNearestPassablePosition();
    }
}

void Game::CollectFood() {
    const Food* food = m_currentWorld->GetFoodAt(m_playerX, m_playerY);
    if (food) {
        // Восстанавливаем показатели
        int oldHP = m_playerHP;
        int oldHunger = m_playerHunger;

        m_playerHunger = min(MAX_HUNGER, m_playerHunger + food->GetHungerRestore());
        m_playerHP = min(MAX_HP, m_playerHP + food->GetHpRestore());

        // ПОЛУЧАЕМ ОПЫТ ЗА ЕДУ
        int xpGained = food->GetExperience();
        GainXP(xpGained);

        // Убираем еду с карты
        m_currentWorld->RemoveFoodAt(m_playerX, m_playerY);

        // Логируем
        Logger::Log("Collected " + food->GetName() +
            " - Hunger: +" + std::to_string(food->GetHungerRestore()) +
            ", HP: +" + std::to_string(food->GetHpRestore()) +
            ", XP: +" + std::to_string(xpGained) + // Добавьте XP
            " | Now: HP=" + std::to_string(m_playerHP) +
            "/" + std::to_string(MAX_HP) +
            ", Hunger=" + std::to_string(m_playerHunger) +
            "/" + std::to_string(MAX_HUNGER) +
            ", XP=" + std::to_string(m_playerXP) + // Добавьте XP
            "/" + std::to_string(m_xpToNextLevel) +
            ", Level=" + std::to_string(m_playerLevel)); // Добавьте уровень
    }
}

/// <summary>
/// Отрисовка
/// </summary>
void Game::Render() {
    static int lastPlayerX = m_playerX;
    static int lastPlayerY = m_playerY;

    m_renderSystem->StartFrame();

    m_renderSystem->DrawWorld(*m_currentWorld);
    m_renderSystem->DrawPlayer(m_playerX, m_playerY, lastPlayerX, lastPlayerY, *m_currentWorld);

    static int uiCounter = 0;
    if (uiCounter++ > UiUpdateInterval) {
        m_renderSystem->DrawUI(*m_currentWorld, m_playerX, m_playerY, m_playerSteps,
            m_playerHP, MAX_HP, m_playerHunger, MAX_HUNGER,
            m_playerXP, m_playerLevel, m_xpToNextLevel); // Добавьте опыт и уровень
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
bool Game::MovePlayer(int dx, int dy) { // ← ИЗМЕНИТЕ НА bool
    int newX = m_playerX + dx;
    int newY = m_playerY + dy;

    // Проверяем границы игрового пространства (без учета границы)
    if (newX >= 0 && newX < m_currentWorld->GetWidth() &&
        newY >= 0 && newY < m_currentWorld->GetHeight()) {

        int targetTile = m_currentWorld->GetTileAt(newX, newY);
        TileType* tile = m_tileManager->GetTileType(targetTile);

        const Food* food = m_currentWorld->GetFoodAt(newX, newY);
        if (food) {
            // Если на клетке есть еда - разрешаем ход независимо от проходимости тайла
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
    int currentTile = m_currentWorld->GetTileAt(m_playerX, m_playerY);
    TileType* tile = m_tileManager->GetTileType(currentTile);

    if (!tile || !tile->IsPassable()) {
        FindNearestPassablePosition();
    }
}

/// <summary>
/// Поиск ближайшего проходимого места
/// </summary>
void Game::FindNearestPassablePosition() {
    for (int radius = 1; radius <= MaxSearchRadius; radius++) {
        for (int dy = -radius; dy <= radius; dy++) {
            for (int dx = -radius; dx <= radius; dx++) {
                if (radius > 1 && abs(dx) < radius && abs(dy) < radius) {
                    continue;
                }

                int checkX = m_playerX + dx;
                int checkY = m_playerY + dy;

                // Проверяем границы игрового пространства
                if (checkX >= 0 && checkX < m_currentWorld->GetWidth() &&
                    checkY >= 0 && checkY < m_currentWorld->GetHeight()) {

                    int tileId = m_currentWorld->GetTileAt(checkX, checkY);
                    TileType* tile = m_tileManager->GetTileType(tileId);

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
    cout << "Using fallback: searching for random passable position..." << endl;

    srand(static_cast<unsigned int>(time(nullptr)));

    for (int attempt = 0; attempt < MaxRandomAttempts; attempt++) {
        // Ищем в игровом пространстве (без границы)
        int randomX = rand() % m_currentWorld->GetWidth();
        int randomY = rand() % m_currentWorld->GetHeight();

        int tileId = m_currentWorld->GetTileAt(randomX, randomY);
        TileType* tile = m_tileManager->GetTileType(tileId);

        if (tile && tile->IsPassable()) {
            cout << "Found random passable position at: " << randomX << ", " << randomY << endl;
            m_playerX = randomX;
            m_playerY = randomY;
            return;
        }
    }

    cout << "Emergency: setting player to position 1,1" << endl;
    m_playerX = EmergencyPositionX;
    m_playerY = EmergencyPositionY;
}

void Game::ShowDeathScreen() {
    m_currentWorld->ClearAllFood();

    system("cls");

    // Ждем для стабилизации
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);

    int screenWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    int screenHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

    // Устанавливаем красный цвет
    SetConsoleTextAttribute(hConsole, 12);

    // Очищаем экран пробелами
    for (int y = 0; y < screenHeight; y++) {
        COORD pos = { 0, static_cast<SHORT>(y) };
        SetConsoleCursorPosition(hConsole, pos);
        cout << std::string(screenWidth, ' ');
    }

    // Простой экран смерти БЕЗ псевдографики
    int startY = screenHeight / 2 - 3;

    COORD titlePos = { static_cast<SHORT>(screenWidth / 2 - 5), static_cast<SHORT>(startY) };
    SetConsoleCursorPosition(hConsole, titlePos);
    cout << "YOU DIED!";

    COORD stepsPos = { static_cast<SHORT>(screenWidth / 2 - 5), static_cast<SHORT>(startY + 2) };
    SetConsoleCursorPosition(hConsole, stepsPos);
    cout << "Steps: " << m_playerSteps;

    COORD levelPos = { static_cast<SHORT>(screenWidth / 2 - 5), static_cast<SHORT>(startY + 3) };
    SetConsoleCursorPosition(hConsole, levelPos);
    cout << "Level: " << m_playerLevel;

    COORD xpPos = { static_cast<SHORT>(screenWidth / 2 - 5), static_cast<SHORT>(startY + 4) };
    SetConsoleCursorPosition(hConsole, xpPos);
    cout << "Total XP: " << m_playerXP;

    COORD exitPos = { static_cast<SHORT>(screenWidth / 2 - 5), static_cast<SHORT>(startY + 4) };
    SetConsoleCursorPosition(hConsole, exitPos);
    m_isRunning = false;

    _getch();

    // Восстанавливаем цвет
    SetConsoleTextAttribute(hConsole, 7);
}

void Game::GainXP(int amount) {
    m_playerXP += amount;
    Logger::Log("Gained " + std::to_string(amount) + " XP! Total: " +
        std::to_string(m_playerXP) + "/" + std::to_string(m_xpToNextLevel));

    CheckLevelUp();
}

void Game::CheckLevelUp() {
    while (m_playerXP >= m_xpToNextLevel) {
        m_playerXP -= m_xpToNextLevel;
        m_playerLevel++;

        // Увеличиваем требования к опыту для следующего уровня
        m_xpToNextLevel = static_cast<int>(m_xpToNextLevel * 1.5f);

        // Бонусы за уровень
        m_playerHP = MAX_HP; // Полное восстановление HP
        m_playerHunger = MAX_HUNGER; // Полное восстановление голода

        Logger::Log("LEVEL UP! Reached level " + std::to_string(m_playerLevel) +
            "! Next level at " + std::to_string(m_xpToNextLevel) + " XP");
    }
}