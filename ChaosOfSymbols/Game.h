#pragma once
#include "World.h"
#include "GameSettings.h"
#include "TileTypeManager.h"
#include "RenderSystem.h"
#include <windows.h>
#include "FoodManager.h"

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
    FoodManager* m_foodManager;

    int m_playerX;
    int m_playerY;

    int m_playerSteps;

    bool m_automatonEnabled;
    int m_actionsSinceLastUpdate;
    static constexpr int ActionsPerUpdate = 1; // Обновлять автомат после каждого действия

    const int MAX_HP = 30;
    const int MAX_HUNGER = 20;
    int m_playerHP;     
    int m_playerHunger; 

    int m_playerXP;          // Текущий опыт
    int m_playerLevel;       // Текущий уровень
    int m_xpToNextLevel;     // Опыт до следующего уровня

public:
    Game();
    ~Game();

    bool Initialize();
    void Run();
    void ProcessInput();
    void Update();
    void Render();
    void Shutdown();

    int GetPlayerSteps() const { return m_playerSteps; }
    int GetPlayerHP() const { return m_playerHP; }       
    int GetPlayerHunger() const { return m_playerHunger; } 
    int GetMaxHP() const { return MAX_HP; }                
    int GetMaxHunger() const { return MAX_HUNGER; }      

private:
    bool MovePlayer(int dx, int dy);
    void EnsureValidPlayerPosition();
    void FindNearestPassablePosition();
    void FindRandomPassablePosition();

    void UpdateCellularAutomaton();

    void ConsumeEnergy();
    void ShowDeathScreen();
    void CollectFood();

    void GainXP(int amount); // Метод для получения опыта
    void CheckLevelUp();     // Проверка повышения уровня
};