#pragma once
#include <vector>
#include <chrono>
#include "World.h"
#include "TileTypeManager.h"

namespace rlutil {
    void setColor(int color);
    void cls();
    void locate(int x, int y);
    void hideCursor();
    int trows();
    int tcols();
}

class RenderSystem {
public:
    // Конструктор, деструктор
    RenderSystem(TileTypeManager* tileManager);
    ~RenderSystem();

    // Публичные методы
    void ClearScreen();
    void DrawWorld(const World& world);
    void DrawUI(const World& world, int posX, int posY, int playerSteps, int playerHP, int playerMaxHP, int playerHunger, int playerMaxHunger,
        int playerXP, int playerLevel, int xpToNextLevel);
    void DrawPlayer(int x, int y, int previousX, int previousY, const World& world);
    void SetScreenSize(int width, int height);
    void StartFrame();
    void EndFrame();
    void LogStats() const;
    void ClearEntireScreen();

    // Геттеры
    int GetScreenWidth() const { return m_screenWidth; }
    int GetScreenHeight() const { return m_screenHeight; }
    double GetCurrentFPS() const { return m_stats.currentFps; }
    double GetAverageFPS() const { return m_stats.averageFps; }

private:
    // Приватные методы
    void InitializePreviousFrame();
    bool NeedsRedraw(int x, int y, int tileId);
    void UpdateFPS();

    // Приватные структуры
    struct RenderStats {
        int framesRendered = 0;
        int tilesDrawn = 0;
        int tilesSkipped = 0;
        std::chrono::steady_clock::time_point lastFpsUpdate;
        std::chrono::steady_clock::time_point frameStart;
        double currentFps = 0.0;
        double averageFps = 0.0;
        double minFps = 1000.0;
        double maxFps = 0.0;
        std::vector<double> fpsHistory;
    };

    // Константы
    static constexpr int DefaultScreenWidth = 80;
    static constexpr int DefaultScreenHeight = 24;
    static constexpr int PlayerColor = 12; // Красный
    static constexpr char PlayerChar = '@';
    static constexpr int UnknownTileColor = 10; // Серый
    static constexpr char UnknownTileChar = '.';
    static constexpr int PlayerTileId = -9999; // Уникальный ID для игрока
    static constexpr int FoodIdOffset = 1000;
   
    // Ппиватные поля
    const int BORDER_TILE_ID = -2;
    const int PLAYER_TILE_ID = -1;
    const int FOOD_TILE_ID_BASE = 1000;
    TileTypeManager* m_tileManager;
    std::vector<std::vector<int>> m_previousFrame;
    int m_screenWidth;
    int m_screenHeight;
    RenderStats m_stats;
};