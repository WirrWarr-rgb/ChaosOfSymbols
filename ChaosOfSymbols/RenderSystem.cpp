#include "RenderSystem.h"
#include "Logger.h"
#include <iostream>
#define NOMINMAX
#include <windows.h>

namespace rlutil {
    void setColor(int color) {
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
    }

    void cls() {
        COORD topLeft = { 0, 0 };
        CONSOLE_SCREEN_BUFFER_INFO screen;
        DWORD written;
        HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);

        GetConsoleScreenBufferInfo(console, &screen);
        DWORD cells = screen.dwSize.X * screen.dwSize.Y;
        FillConsoleOutputCharacterA(console, ' ', cells, topLeft, &written);
        FillConsoleOutputAttribute(console, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE,
            cells, topLeft, &written);
        SetConsoleCursorPosition(console, topLeft);
    }

    void locate(int x, int y) {
        COORD coord;
        coord.X = static_cast<SHORT>(x);
        coord.Y = static_cast<SHORT>(y);
        SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
    }

    void hideCursor() {
        CONSOLE_CURSOR_INFO cursorInfo;
        cursorInfo.dwSize = 100;
        cursorInfo.bVisible = FALSE;
        SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
    }

    int trows() {
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
        return csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    }

    int tcols() {
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
        return csbi.srWindow.Right - csbi.srWindow.Left + 1;
    }
}

RenderSystem::RenderSystem(TileTypeManager* tileManager)
    : m_tileManager(tileManager) {
    rlutil::hideCursor();
    m_screenWidth = 80;
    m_screenHeight = 24;
    InitializePreviousFrame();
}

RenderSystem::~RenderSystem() {
    COORD size = { 80, 25 };
    SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), size);

    SMALL_RECT rect = { 0, 0, 79, 24 };
    SetConsoleWindowInfo(GetStdHandle(STD_OUTPUT_HANDLE), TRUE, &rect);
}

void RenderSystem::SetScreenSize(int width, int height) {
    m_screenWidth = width;
    m_screenHeight = height;

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    COORD bufferSize = { static_cast<SHORT>(width), static_cast<SHORT>(height + 1) };
    SetConsoleScreenBufferSize(hConsole, bufferSize);

    SMALL_RECT windowSize = { 0, 0, static_cast<SHORT>(width - 1), static_cast<SHORT>(height) };
    SetConsoleWindowInfo(hConsole, TRUE, &windowSize);

    InitializePreviousFrame();
    ClearScreen();
}

void RenderSystem::InitializePreviousFrame() {
    m_previousFrame.clear();
    m_previousFrame.resize(m_screenHeight, std::vector<int>(m_screenWidth, -1));
}

void RenderSystem::ClearScreen() {
    rlutil::cls();
    InitializePreviousFrame();
}

void RenderSystem::DrawWorld(const World& world) {
    int worldWidth = world.GetWidth();
    int worldHeight = world.GetHeight();

    if (worldWidth != m_screenWidth || worldHeight != m_screenHeight) {
        SetScreenSize(worldWidth, worldHeight);
        ClearScreen();
    }

    static bool firstDraw = true;
    if (firstDraw) {
        Logger::Log("First draw - world size: " + std::to_string(worldWidth) + "x" + std::to_string(worldHeight));
        for (int y = 0; y < std::min(3, worldHeight); y++) {
            for (int x = 0; x < std::min(3, worldWidth); x++) {
                int tileId = world.GetTileAt(x, y);
                TileType* tile = m_tileManager->GetTileType(tileId);
                Logger::Log("Tile at " + std::to_string(x) + "," + std::to_string(y) +
                    ": ID=" + std::to_string(tileId) +
                    ", TilePtr=" + (tile ? tile->GetName() : "NULL"));
            }
        }
        firstDraw = false;
    }

    for (int y = 0; y < worldHeight; y++) {
        for (int x = 0; x < worldWidth; x++) {
            int tileId = world.GetTileAt(x, y);

            if (NeedsRedraw(x, y, tileId)) {
                rlutil::locate(x, y);
                TileType* tile = m_tileManager->GetTileType(tileId);

                if (tile) {
                    rlutil::setColor(tile->GetColor());
                    std::cout << tile->GetCharacter();
                }
                else {
                    rlutil::setColor(8);
                    std::cout << '?';
                    if (x < 5 && y < 5) {
                        Logger::Log("WARNING: Unknown tile ID " + std::to_string(tileId) +
                            " at position " + std::to_string(x) + "," + std::to_string(y));
                    }
                }

                m_previousFrame[y][x] = tileId;
            }
        }
    }
    rlutil::setColor(15);
}

bool RenderSystem::NeedsRedraw(int x, int y, int tileId) {
    if (x < 0 || x >= m_screenWidth || y < 0 || y >= m_screenHeight)
        return false;

    return m_previousFrame[y][x] != tileId;
}

void RenderSystem::DrawPlayer(int x, int y, int previousX, int previousY, const World& world) {
    if (previousX >= 0 && previousX < m_screenWidth &&
        previousY >= 0 && previousY < m_screenHeight) {

        rlutil::locate(previousX, previousY);
        int tileId = world.GetTileAt(previousX, previousY);
        TileType* tile = m_tileManager->GetTileType(tileId);

        if (tile) {
            rlutil::setColor(tile->GetColor());
            std::cout << tile->GetCharacter();
            m_previousFrame[previousY][previousX] = tileId;
        }
    }

    if (x >= 0 && x < m_screenWidth && y >= 0 && y < m_screenHeight) {
        rlutil::locate(x, y);
        rlutil::setColor(12);
        std::cout << '@';
        m_previousFrame[y][x] = -2;
    }

    rlutil::setColor(15);
}

void RenderSystem::DrawUI(const World& world, int posX, int posY) {
    rlutil::locate(0, m_screenHeight);
    for (int i = 0; i < m_screenWidth; i++) {
        std::cout << ' ';
    }

    rlutil::locate(0, m_screenHeight);
    std::cout << "Pos: " << posX << "," << posY;
    std::cout << " | Seed: " << world.GetCurrentSeed();
    std::cout << " | Controls: WASD-move, Q-quit, R-regenerate";
}