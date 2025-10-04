#pragma once
#include "World.h"
#include "TileTypeManager.h"
#include <vector>

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
    TileTypeManager* m_tileManager;
    std::vector<std::vector<int>> m_previousFrame;
    int m_screenWidth;
    int m_screenHeight;

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

private:
    void InitializePreviousFrame();
    bool NeedsRedraw(int x, int y, int tileId);
};