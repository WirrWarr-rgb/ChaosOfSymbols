#pragma once
#include "World.h"
#include "GameSettings.h"
#include "TileTypeManager.h"
#include "RenderSystem.h"
#include <windows.h>

class Game {
private:
    // Константы для настройки игры
    static constexpr const char* LogFile = "config/debug.log";
    static constexpr const char* WorldConfigFile = "config/world_gen.cfg";
    static constexpr const char* WorldSaveFile = "saves/world.dat";

    // Константы времени (в миллисекундах)
    static constexpr int FrameDelayMs = 50;        // 20 FPS
    static constexpr int MoveCooldownMs = 100;     // Задержка между движениями

    // Константы игровой логики
    static constexpr int DefaultPlayerX = 10;
    static constexpr int DefaultPlayerY = 10;
    static constexpr int UiUpdateInterval = 6;
    static constexpr int MaxSearchRadius = 20;
    static constexpr int MaxRandomAttempts = 100;
    static constexpr int EmergencyPositionX = 1;
    static constexpr int EmergencyPositionY = 1;

    bool m_isRunning;
    World* m_currentWorld;
    GameSettings* m_settings;
    RenderSystem* m_renderSystem;
    TileTypeManager* m_tileManager;

    int m_playerX;
    int m_playerY;

    bool m_automatonEnabled;
    int m_actionsSinceLastUpdate;
    static constexpr int ActionsPerUpdate = 1; // Обновлять автомат после каждого действия

public:
    Game();
    ~Game();

    bool Initialize();
    void Run();
    void ProcessInput();
    void Update();
    void Render();
    void Shutdown();

private:
    void MovePlayer(int dx, int dy);
    void EnsureValidPlayerPosition();
    void FindNearestPassablePosition();
    void FindRandomPassablePosition();

    void UpdateCellularAutomaton();
};