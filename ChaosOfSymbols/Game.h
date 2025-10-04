#pragma once
#include "World.h"
#include "GameSettings.h"
#include "TileTypeManager.h"
#include "RenderSystem.h"
#include <windows.h>

class Game {
private:
    bool m_isRunning;
    World* m_currentWorld;
    GameSettings* m_settings;
    RenderSystem* m_renderSystem;
    TileTypeManager* m_tileManager;

    int m_playerX;
    int m_playerY;

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
};