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

static bool rPressed = false;

using namespace std;

Game::Game()
    : m_isRunning(false), m_currentWorld(nullptr),
    m_settings(nullptr), m_renderSystem(nullptr),
    m_tileManager(nullptr), m_playerX(DefaultPlayerX), m_playerY(DefaultPlayerY) {}

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

    Logger::Log("Testing config-based generation...");

    m_currentWorld = new World();
    m_currentWorld->SetTileManager(m_tileManager);

    // Включаем клеточный автомат
    m_currentWorld->SetAutomatonEnabled(true);

    m_renderSystem = new RenderSystem(m_tileManager);

    m_currentWorld->GenerateFromConfig(WorldConfigFile);

    m_renderSystem->SetScreenSize(m_currentWorld->GetWidth(), m_currentWorld->GetHeight());

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
        Logger::Log("Player moved - updating cellular automaton");
        m_currentWorld->UpdateCellularAutomaton();
    }

    if (GetAsyncKeyState('Q') & 0x8000) {
        m_isRunning = false;
    }

    if (GetAsyncKeyState('R') & 0x8000) {
        if (!rPressed) {
            Logger::Log("Regenerating world from config...");
            m_currentWorld->GenerateFromConfig("config/world_gen.cfg");

            m_renderSystem->SetScreenSize(m_currentWorld->GetWidth(), m_currentWorld->GetHeight());

            EnsureValidPlayerPosition();
            rPressed = true;
        }
    }
    else {
        rPressed = false;
    }
}

/// <summary>
/// Обновление состояния
/// </summary>
void Game::Update() {
    int currentTile = m_currentWorld->GetTileAt(m_playerX, m_playerY);
    TileType* tile = m_tileManager->GetTileType(currentTile);

    if (tile && !tile->IsPassable()) {
        FindNearestPassablePosition();
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
        m_renderSystem->DrawUI(*m_currentWorld, m_playerX, m_playerY);
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
void Game::MovePlayer(int dx, int dy) {
    int newX = m_playerX + dx;
    int newY = m_playerY + dy;

    if (newX > 0 && newX < m_currentWorld->GetWidth() - 1 &&
        newY > 0 && newY < m_currentWorld->GetHeight() - 1) {

        int targetTile = m_currentWorld->GetTileAt(newX, newY);
        TileType* tile = m_tileManager->GetTileType(targetTile);

        if (tile && tile->IsPassable()) {
            m_playerX = newX;
            m_playerY = newY;
        }
    }
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