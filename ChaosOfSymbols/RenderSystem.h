#pragma once
#include "World.h"
#include "TileTypeManager.h"
#include <vector>
#include <chrono>

namespace rlutil {
    void setColor(int color);
    void cls();
    void locate(int x, int y);
    void hideCursor();
    int trows();
    int tcols();
}

class RenderSystem {
private:
    // Константы
    static constexpr int DefaultScreenWidth = 80;
    static constexpr int DefaultScreenHeight = 24;
    static constexpr int PlayerColor = 12; // Красный
    static constexpr char PlayerChar = '@';
    static constexpr int UnknownTileColor = 8; // Серый
    static constexpr char UnknownTileChar = '?';
    static constexpr int PlayerTileId = -2; // Специальный ID для игрока

    // Статистика
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


    TileTypeManager* m_tileManager;
    std::vector<std::vector<int>> m_previousFrame;
    int m_screenWidth;
    int m_screenHeight;
    RenderStats m_stats;

public:
    RenderSystem(TileTypeManager* tileManager);
    ~RenderSystem();

    void ClearScreen();
    void DrawWorld(const World& world);
    void DrawUI(const World& world, int posX, int posY);
    void DrawPlayer(int x, int y, int previousX, int previousY, const World& world);

    void SetScreenSize(int width, int height);
    int GetScreenWidth() const { return m_screenWidth; }
    int GetScreenHeight() const { return m_screenHeight; }

    // Методы для статистики
    void StartFrame();
    void EndFrame();
    double GetCurrentFPS() const { return m_stats.currentFps; }
    double GetAverageFPS() const { return m_stats.averageFps; }
    void LogStats() const;

private:
    void InitializePreviousFrame();
    bool NeedsRedraw(int x, int y, int tileId);
    void UpdateFPS();
};