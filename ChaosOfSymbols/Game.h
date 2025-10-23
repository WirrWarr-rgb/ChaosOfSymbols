#pragma once
#include <windows.h>
#include <memory>
#include "World.h"
#include "RenderSystem.h"
#include "ConfigManager.h"

class Game {
public:
    // Конструктор, деструктор
    Game();
    ~Game();

    // Публичные методы
    bool Initialize();
    void Run();
    void ProcessInput();
    void Update();
    void Render();
    void Shutdown();

    // Геттеры
    int GetPlayerSteps() const { return m_playerSteps; }
    int GetPlayerHP() const { return m_playerHP; }
    int GetPlayerHunger() const { return m_playerHunger; }
    int GetMaxHP() const { return MAX_HP; }
    int GetMaxHunger() const { return MAX_HUNGER; }

private:
    // Приватные методы
    bool MovePlayer(int dx, int dy);
    void EnsureValidPlayerPosition();
    void FindNearestPassablePosition();
    void FindRandomPassablePosition();

    void ConsumeEnergy();
    void ShowDeathScreen();
    void CollectFood();

    void GainXP(int amount);
    void CheckLevelUp();

    void OnTilesChanged();
    void OnFoodChanged();
    void OnAutomatonRulesChanged();

    // Константы
    static constexpr const char* LogFile = "config/debug.log";
    static constexpr int FrameDelayMs = 33;        // 20 FPS
    static constexpr int MoveCooldownMs = 66;     // Задержка между движениями
    static constexpr int DefaultPlayerX = 10;
    static constexpr int DefaultPlayerY = 10;
    static constexpr int UiUpdateInterval = 6;
    static constexpr int MaxSearchRadius = 20;
    static constexpr int MaxRandomAttempts = 100;
    static constexpr int EmergencyPositionX = 1;
    static constexpr int EmergencyPositionY = 1;
   
    // Приватные поля
    bool m_isRunning;
    World* m_currentWorld;
    RenderSystem* m_renderSystem;
    std::unique_ptr<ConfigManager> m_configManager;
    int m_playerX;
    int m_playerY;
    int m_playerSteps;
    bool m_automatonEnabled;
    int m_actionsSinceLastUpdate;
    static constexpr int ActionsPerUpdate = 1;
    const int MAX_HP = 30;
    const int MAX_HUNGER = 20;
    int m_playerHP;
    int m_playerHunger;
    std::unordered_map<int, int> m_foodEaten;
    int m_totalXP;
    int m_playerXP;
    int m_playerLevel;
    int m_xpToNextLevel;
};