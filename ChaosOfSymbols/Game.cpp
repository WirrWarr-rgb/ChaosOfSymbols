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
    m_renderSystem(nullptr),
    m_playerX(DefaultPlayerX), m_playerY(DefaultPlayerY), m_playerSteps(0),
    m_playerHP(MAX_HP), m_playerHunger(MAX_HUNGER),
    m_playerXP(0), m_playerLevel(1), m_xpToNextLevel(100),
    m_totalXP(0), // Инициализируем общий опыт
    m_foodEaten()
{
    // Инициализируем счетчики для всех известных типов еды нулями
    for (int i = 1; i <= 4; i++) {
        m_foodEaten[i] = 0;
    }
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
    Logger::Log("=== GAME INITIALIZATION STARTED ===\n");

    // Инициализируем менеджер конфигураций
    m_configManager = std::make_unique<ConfigManager>();
    if (!m_configManager->Initialize()) {
        Logger::Log("ERROR: Failed to initialize config manager!");
        return false;
    }

    // Настраиваем обработчики изменений
    m_configManager->OnTilesChanged = [this]() { this->OnTilesChanged(); };
    m_configManager->OnFoodChanged = [this]() { this->OnFoodChanged(); };
    m_configManager->OnAutomatonRulesChanged = [this]() { this->OnAutomatonRulesChanged(); };

    // Получаем менеджеры из ConfigManager
    TileTypeManager* tileManager = m_configManager->GetTileManager();
    FoodManager* foodManager = m_configManager->GetFoodManager();

    // Остальная инициализация остается похожей, но используем менеджеры из ConfigManager
    m_currentWorld = new World();
    m_currentWorld->SetTileManager(tileManager);
    m_currentWorld->SetFoodManager(foodManager);
    m_currentWorld->SetAutomatonEnabled(true);

    // Устанавливаем конфиг автомата
    m_currentWorld->SetAutomatonConfig(m_configManager->GetAutomatonConfig());

    m_renderSystem = new RenderSystem(tileManager);

    m_currentWorld->GenerateFromConfig();

    m_renderSystem->SetScreenSize(m_currentWorld->GetTotalWidth(), m_currentWorld->GetTotalHeight());

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

    delete m_currentWorld;
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
        return;
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
            m_playerHP = MAX_HP;
            m_playerHunger = MAX_HUNGER;
            m_totalXP = 0; // Сбрасываем общий опыт

            // Сбрасываем счетчики еды
            for (auto& pair : m_foodEaten) {
                pair.second = 0;
            }

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

    // Обновляем мониторинг файлов конфигурации
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

    // Получаем tileManager из configManager
    TileTypeManager* tileManager = m_configManager->GetTileManager();
    int currentTile = m_currentWorld->GetTileAt(m_playerX, m_playerY);
    TileType* tile = tileManager->GetTileType(currentTile);  // Используем tileManager из configManager

    if (tile && !tile->IsPassable()) {
        FindNearestPassablePosition();
    }
}

void Game::CollectFood() {
    const Food* food = m_currentWorld->GetFoodAt(m_playerX, m_playerY);
    if (food) {
        // Восстанавливаем показатели
        m_foodEaten[food->GetId()]++;

        int oldHP = m_playerHP;
        int oldHunger = m_playerHunger;

        m_playerHunger = min(MAX_HUNGER, m_playerHunger + food->GetHungerRestore());
        m_playerHP = min(MAX_HP, m_playerHP + food->GetHpRestore());

        int xpGained = food->GetExperience();
        GainXP(xpGained);

        m_currentWorld->RemoveFoodAt(m_playerX, m_playerY);

        Logger::Log("Collected " + food->GetName() +
            " - Hunger: +" + std::to_string(food->GetHungerRestore()) +
            ", HP: +" + std::to_string(food->GetHpRestore()) +
            ", XP: +" + std::to_string(xpGained) +
            " | Now: HP=" + std::to_string(m_playerHP) +
            "/" + std::to_string(MAX_HP) +
            ", Hunger=" + std::to_string(m_playerHunger) +
            "/" + std::to_string(MAX_HUNGER) +
            ", XP=" + std::to_string(m_playerXP) +
            "/" + std::to_string(m_xpToNextLevel) +
            ", Level=" + std::to_string(m_playerLevel));
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

    // Получаем tileManager из configManager
    TileTypeManager* tileManager = m_configManager->GetTileManager();

    // Проверяем границы игрового пространства (без учета границы)
    if (newX >= 0 && newX < m_currentWorld->GetWidth() &&
        newY >= 0 && newY < m_currentWorld->GetHeight()) {

        int targetTile = m_currentWorld->GetTileAt(newX, newY);
        TileType* tile = tileManager->GetTileType(targetTile);  // Используем tileManager из configManager

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
    // Получаем tileManager из configManager
    TileTypeManager* tileManager = m_configManager->GetTileManager();

    int currentTile = m_currentWorld->GetTileAt(m_playerX, m_playerY);
    TileType* tile = tileManager->GetTileType(currentTile);  // Используем tileManager из configManager

    if (!tile || !tile->IsPassable()) {
        FindNearestPassablePosition();
    }
}

/// <summary>
/// Поиск ближайшего проходимого места
/// </summary>
void Game::FindNearestPassablePosition() {
    // Получаем tileManager из configManager
    TileTypeManager* tileManager = m_configManager->GetTileManager();

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
                    TileType* tile = tileManager->GetTileType(tileId);  // Используем tileManager из configManager

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
    // Получаем tileManager из configManager
    TileTypeManager* tileManager = m_configManager->GetTileManager();

    cout << "Using fallback: searching for random passable position..." << endl;

    srand(static_cast<unsigned int>(time(nullptr)));

    for (int attempt = 0; attempt < MaxRandomAttempts; attempt++) {
        // Ищем в игровом пространстве (без границы)
        int randomX = rand() % m_currentWorld->GetWidth();
        int randomY = rand() % m_currentWorld->GetHeight();

        int tileId = m_currentWorld->GetTileAt(randomX, randomY);
        TileType* tile = tileManager->GetTileType(tileId);  // Используем tileManager из configManager

        if (tile && tile->IsPassable()) {
            m_playerX = randomX;
            m_playerY = randomY;
            return;
        }
    }

    m_playerX = EmergencyPositionX;
    m_playerY = EmergencyPositionY;
}

void Game::ShowDeathScreen() {
    // Останавливаем игру СРАЗУ
    m_isRunning = false;

    // Даем время завершиться текущему циклу рендеринга
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // МНОГОКРАТНАЯ и АГРЕССИВНАЯ очистка
    for (int i = 0; i < 5; i++) {
        system("cls");
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    // Полная ручная очистка консоли
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);

    COORD topLeft = { 0, 0 };
    DWORD consoleSize = csbi.dwSize.X * csbi.dwSize.Y;
    DWORD written;

    // Очищаем ВСЕ символы
    FillConsoleOutputCharacterA(hConsole, ' ', consoleSize, topLeft, &written);
    // Очищаем ВСЕ атрибуты цвета
    FillConsoleOutputAttribute(hConsole, 7, consoleSize, topLeft, &written);
    // Курсор в начало
    SetConsoleCursorPosition(hConsole, topLeft);

    // Очищаем еду
    m_currentWorld->ClearAllFood();

    // Устанавливаем красный цвет
    SetConsoleTextAttribute(hConsole, 7);
    cout << "==========================================\n";

    SetConsoleTextAttribute(hConsole, 12);
    cout << "             YOU DIED! Unlucky :)\n";

    SetConsoleTextAttribute(hConsole, 7);
    cout << "==========================================\n";
    cout << "Steps: " << m_playerSteps << "\n";
    cout << "Total XP: " << m_totalXP << "\n";
    cout << "Current Level: " << m_playerLevel << "\n";
    cout << "Current XP: " << m_playerXP << "\n";
    cout << "==========================================\n";
    cout << "Food consumed:\n";

    // Получаем менеджер еды для получения информации о типах еды
    FoodManager* foodManager = m_configManager->GetFoodManager();

    // Для каждого типа еды в конфиге выводим статистику
    for (int foodId = 1; foodId <= 4; foodId++) { // предполагая, что ID от 1 до 4
        const Food* foodType = foodManager->GetFood(foodId);
        if (foodType) {
            int count = m_foodEaten[foodId];

            // Устанавливаем цвет еды
            SetConsoleTextAttribute(hConsole, foodType->GetColor());

            cout << foodType->GetName() << ": " << count;

            // Возвращаем белый цвет для остального текста
            SetConsoleTextAttribute(hConsole, 7);
            cout << "\n";
        }
    }

    // Возвращаем красный цвет для завершающей линии
    SetConsoleTextAttribute(hConsole, 7);
    cout << "==========================================\n";
    SetConsoleTextAttribute(hConsole, 7);
    cout << "Press ESC to exit..." << "\n";

    while (true) {
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // Восстанавливаем нормальный цвет
    SetConsoleTextAttribute(hConsole, 7);

    // ЗАКРЫВАЕМ ПРОГРАММУ СРАЗУ после нажатия клавиши
    exit(0);  // ← ДОБАВЬТЕ ЭТУ СТРОЧКУ
}

void Game::GainXP(int amount) {
    m_playerXP += amount;
    m_totalXP += amount; // Добавляем к общему опыту

    Logger::Log("Gained " + std::to_string(amount) + " XP! Total: " +
        std::to_string(m_playerXP) + "/" + std::to_string(m_xpToNextLevel) +
        ", Lifetime: " + std::to_string(m_totalXP));

    CheckLevelUp();
}

void Game::CheckLevelUp() {
    while (m_playerXP >= m_xpToNextLevel) {
        m_playerXP -= m_xpToNextLevel;
        m_playerLevel++;

        m_xpToNextLevel = static_cast<int>(m_xpToNextLevel * 1.5f);

        m_playerHP = MAX_HP;
        m_playerHunger = MAX_HUNGER;

        Logger::Log("LEVEL UP! Reached level " + std::to_string(m_playerLevel) +
            "! Next level at " + std::to_string(m_xpToNextLevel) + " XP");
    }
}

// Game.cpp - обновляем методы
void Game::OnTilesChanged() {
    Logger::Log("Tile configurations changed - updating world...");

    if (!m_currentWorld || !m_configManager->GetTileManager()) return;

    // Обновляем внешний вид всех тайлов (цвет, символы, проходимость)
    m_currentWorld->UpdateTileAppearance();

    // Проверяем позицию игрока - вдруг он стоит на ставшем непроходимом тайле
    EnsureValidPlayerPosition();

    // Обновляем рендер-систему с новым tile manager
    if (m_renderSystem) {
        // RenderSystem уже использует правильный tileManager через указатель
        // Просто принудительно перерисовываем экран
        m_renderSystem->ClearScreen();
    }

    Logger::Log("Tile changes applied successfully");
}

void Game::OnFoodChanged() {
    Logger::Log("Food configurations changed - updating food...");

    if (m_currentWorld) {
        // Полностью очищаем и респавним еду с новыми настройками
        m_currentWorld->ClearAllFood();

        // Респавним начальное количество еды
        int initialFoodCount = (m_currentWorld->GetWidth() * m_currentWorld->GetHeight()) / 10;
        initialFoodCount = std::min<int>(initialFoodCount, 30);
        m_currentWorld->SpawnRandomFood(initialFoodCount);

        Logger::Log("Food updated with new configurations");
    }
}

void Game::OnAutomatonRulesChanged() {
    Logger::Log("Automaton rules changed - updating rules...");

    if (m_currentWorld && m_configManager->GetAutomatonConfig()) {
        // Устанавливаем новые правила
        m_currentWorld->SetAutomatonConfig(m_configManager->GetAutomatonConfig());

        // Немедленно применяем новые правила к существующему миру
        m_currentWorld->UpdateCellularAutomaton();

        // Проверяем позицию игрока после изменений
        EnsureValidPlayerPosition();

        Logger::Log("New automaton rules applied successfully");
    }
}