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
static bool kPressed = false;
static bool lPressed = false;

Game::Game()
    : m_isRunning(false), m_currentWorld(nullptr),
    m_settings(nullptr), m_renderSystem(nullptr),
    m_tileManager(nullptr), m_playerX(10), m_playerY(10) {}

Game::~Game() {
    Shutdown();
}

bool Game::Initialize() {
    Logger::Initialize("debug.log");
    Logger::Log("=== GAME INITIALIZATION STARTED ===");

    std::cout << "Initializing ASCII Game..." << std::endl;

    m_settings = new GameSettings();
    m_settings->LoadFromFile();

    m_tileManager = new TileTypeManager();

    Logger::Log("Attempting to load tile types...");
    if (!m_tileManager->LoadFromFile()) {
        Logger::Log("ERROR: Failed to load tile types!");
        std::cout << "ERROR: Failed to load tile types!" << std::endl;
        return false;
    }
    else {
        Logger::Log("Tile types loaded successfully");
        std::cout << "Tile types loaded successfully" << std::endl;

        Logger::Log("Number of loaded tiles: " + std::to_string(m_tileManager->GetTileCount()));
        std::cout << "Number of loaded tiles: " << m_tileManager->GetTileCount() << std::endl;

        for (int i = 0; i <= 7; i++) {
            TileType* tile = m_tileManager->GetTileType(i);
            if (tile) {
                Logger::Log("Tile " + std::to_string(i) + ": " + tile->GetName() + " ('" + std::string(1, tile->GetCharacter()) + "')");
                std::cout << "Tile " << i << ": " << tile->GetName() << " ('" << tile->GetCharacter() << "')" << std::endl;
            }
            else {
                Logger::Log("WARNING: Tile " + std::to_string(i) + " not found!");
                std::cout << "WARNING: Tile " << i << " not found!" << std::endl;
            }
        }
    }

    Logger::Log("Testing config-based generation...");

    m_currentWorld = new World();

    m_renderSystem = new RenderSystem(m_tileManager);

    m_currentWorld->GenerateFromConfig("config/world_gen.cfg");

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

    std::cout << "Shutting down game..." << std::endl;

    if (m_settings) {
        m_settings->SaveToFile();
        delete m_settings;
    }

    delete m_currentWorld;
    delete m_tileManager;
    delete m_renderSystem;

    std::cout << "Game shutdown complete." << std::endl;
    Logger::Log("=== GAME SHUTDOWN COMPLETED ===");

    Logger::Close();
}

void Game::Run() {
    auto lastTime = std::chrono::steady_clock::now();

    while (m_isRunning) {
        auto currentTime = std::chrono::steady_clock::now();
        auto deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastTime);

        if (deltaTime.count() < 50) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50 - deltaTime.count()));
            continue;
        }

        lastTime = currentTime;

        ProcessInput();
        Update();
        Render();
    }
}

void Game::ProcessInput() {
    static auto lastMoveTime = std::chrono::steady_clock::now();
    auto currentTime = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastMoveTime);

    if (elapsed.count() < 100) {
        return;
    }

    if (GetAsyncKeyState('W') & 0x8000 || GetAsyncKeyState(VK_UP) & 0x8000) {
        MovePlayer(0, -1);
        lastMoveTime = currentTime;
    }
    if (GetAsyncKeyState('S') & 0x8000 || GetAsyncKeyState(VK_DOWN) & 0x8000) {
        MovePlayer(0, 1);
        lastMoveTime = currentTime;
    }
    if (GetAsyncKeyState('A') & 0x8000 || GetAsyncKeyState(VK_LEFT) & 0x8000) {
        MovePlayer(-1, 0);
        lastMoveTime = currentTime;
    }
    if (GetAsyncKeyState('D') & 0x8000 || GetAsyncKeyState(VK_RIGHT) & 0x8000) {
        MovePlayer(1, 0);
        lastMoveTime = currentTime;
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

    if (GetAsyncKeyState('K') & 0x8000) {
        if (!kPressed) {
            m_currentWorld->SaveToFile("saves/world.dat");
            kPressed = true;
        }
    }
    else {
        kPressed = false;
    }

    if (GetAsyncKeyState('L') & 0x8000) {
        if (!lPressed) {
            m_currentWorld->LoadFromFile("saves/world.dat");
            EnsureValidPlayerPosition();
            lPressed = true;
        }
    }
    else {
        lPressed = false;
    }
}

void Game::Update() {
    int currentTile = m_currentWorld->GetTileAt(m_playerX, m_playerY);
    TileType* tile = m_tileManager->GetTileType(currentTile);

    if (tile && !tile->IsPassable()) {
        FindNearestPassablePosition();
    }
}

void Game::Render() {
    static int lastPlayerX = m_playerX;
    static int lastPlayerY = m_playerY;

    m_renderSystem->DrawWorld(*m_currentWorld);

    m_renderSystem->DrawPlayer(m_playerX, m_playerY, lastPlayerX, lastPlayerY, *m_currentWorld);

    static int uiCounter = 0;
    if (uiCounter++ > 5) {
        m_renderSystem->DrawUI(*m_currentWorld, m_playerX, m_playerY);
        uiCounter = 0;
    }

    lastPlayerX = m_playerX;
    lastPlayerY = m_playerY;
}

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

void Game::EnsureValidPlayerPosition() {
    int currentTile = m_currentWorld->GetTileAt(m_playerX, m_playerY);
    TileType* tile = m_tileManager->GetTileType(currentTile);

    if (!tile || !tile->IsPassable()) {
        FindNearestPassablePosition();
    }
}

void Game::FindNearestPassablePosition() {
    std::cout << "Player stuck in impassable tile! Finding nearest passable position..." << std::endl;

    const int maxRadius = 10;

    for (int radius = 1; radius <= maxRadius; radius++) {
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
                        std::cout << "Found passable position at: " << checkX << ", " << checkY << std::endl;
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

void Game::FindRandomPassablePosition() {
    std::cout << "Using fallback: searching for random passable position..." << std::endl;

    srand(static_cast<unsigned int>(time(nullptr)));

    for (int attempt = 0; attempt < 100; attempt++) {
        int randomX = rand() % m_currentWorld->GetWidth();
        int randomY = rand() % m_currentWorld->GetHeight();

        int tileId = m_currentWorld->GetTileAt(randomX, randomY);
        TileType* tile = m_tileManager->GetTileType(tileId);

        if (tile && tile->IsPassable()) {
            std::cout << "Found random passable position at: " << randomX << ", " << randomY << std::endl;
            m_playerX = randomX;
            m_playerY = randomY;
            return;
        }
    }

    std::cout << "Emergency: setting player to position 1,1" << std::endl;
    m_playerX = 1;
    m_playerY = 1;
}