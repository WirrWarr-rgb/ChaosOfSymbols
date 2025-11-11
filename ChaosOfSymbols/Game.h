#pragma once
#include <windows.h>
#include <memory>
#include <unordered_map>
#include "World.h"
#include "RenderSystem.h"
#include "ConfigManager.h"
#include "PlayerConfig.h"
#include "MainMenu.h"
#include "SaveSystem.h" 
#include "PauseMenu.h" 

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
    void SetDefaultConsoleSize();

    // Геттеры
    int GetPlayerSteps() const { return m_playerSteps; }
    int GetPlayerHP() const { return m_playerHP; }
    int GetPlayerHunger() const { return m_playerHunger; }
    int GetMaxHP() const {
        return m_playerConfig ? m_playerConfig->GetMaxHP() : 100;
    }
    int GetMaxHunger() const {
        return m_playerConfig ? m_playerConfig->GetMaxHunger() : 100;
    }

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

    void InitializeMainMenu();
    void RunMainMenu();
    void StartGameFromMenu();
    void InitializeSaveSelection(GameMode mode);
    void RunSaveSelection();
    void StartGameFromSave(GameMode mode, int slot);

    void LoadWorldFromSave(const WorldEditorConfig& config);

    void ReturnToMainMenu();

    // Константы
    static constexpr const char* LogFile = "config/debug.log";
    static constexpr int FrameDelayMs = 33;        // 20 FPS
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
    int m_playerHP;
    int m_playerHunger;
    std::unordered_map<int, int> m_foodEaten;
    int m_totalXP;
    int m_playerXP;
    int m_playerLevel;
    int m_xpToNextLevel;
    PlayerConfig* m_playerConfig;
    std::unique_ptr<MainMenu> m_mainMenu;
    bool m_inMainMenu;
    std::unique_ptr<SaveSystem> m_saveSystem;
    std::unique_ptr<SaveSelectionMenu> m_saveSelectionMenu;
    bool m_inSaveSelection;

    std::unique_ptr<PauseMenu> m_pauseMenu;
    bool m_isPaused;

    void HandlePauseInput();
    void RunPauseMenu();
};