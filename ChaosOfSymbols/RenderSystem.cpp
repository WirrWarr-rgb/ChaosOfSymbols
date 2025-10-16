#include "RenderSystem.h"
#include "Logger.h"
#include <iostream>
#include <algorithm>
#define NOMINMAX
#include <windows.h>
#include <chrono>

namespace rlutil {
    /// <summary>
    /// ��������� ����� ������
    /// </summary>
    /// <param name="color">����</param>
    void setColor(int color) {
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
    }

    /// <summary>
    /// ������� ������
    /// </summary>
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

    /// <summary>
    /// ����������� ������� � ���������� X.Y.
    /// </summary>
    /// <param name="x">���������� X</param>
    /// <param name="y">���������� Y</param>
    void locate(int x, int y) {
        COORD coord;
        coord.X = static_cast<SHORT>(x);
        coord.Y = static_cast<SHORT>(y);
        SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
    }

    /// <summary>
    /// ������� �������
    /// </summary>
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

/// <summary>
/// �����������: �������������
/// </summary>
RenderSystem::RenderSystem(TileTypeManager* tileManager)
    : m_tileManager(tileManager) {
    rlutil::hideCursor();
    m_screenWidth = DefaultScreenWidth;
    m_screenHeight = DefaultScreenHeight;
    InitializePreviousFrame();

    // ������������� ����������
    m_stats.lastFpsUpdate = std::chrono::steady_clock::now();
    m_stats.fpsHistory.reserve(60); // ������ ������� �� ��������� 60 ������
}

/// <summary>
/// ����������: �������������� �������
/// </summary>
RenderSystem::~RenderSystem() {
    COORD size = { DefaultScreenWidth, DefaultScreenHeight-1 };
    SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), size);

    SMALL_RECT rect = { 0, 0, DefaultScreenWidth-1, DefaultScreenHeight-1 };
    SetConsoleWindowInfo(GetStdHandle(STD_OUTPUT_HANDLE), TRUE, &rect);
}


/// <summary>
/// ���������� �������� ������
/// </summary>
/// <param name="width">������</param>
/// <param name="height">������</param>
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

/// <summary>
/// ������� ������� �����������: ����������� ������ ������������ ��������
/// </summary>
void RenderSystem::InitializePreviousFrame() {
    m_previousFrame.clear();
    m_previousFrame.resize(m_screenHeight, std::vector<int>(m_screenWidth, -1));
}

/// <summary>
/// ������� ������� �����������: �������� ��������� �� ����
/// </summary>
bool RenderSystem::NeedsRedraw(int x, int y, int tileId) {
    if (x < 0 || x >= m_screenWidth || y < 0 || y >= m_screenHeight)
        return false;

    return m_previousFrame[y][x] != tileId;
}

/// <summary>
/// ������� �������
/// </summary>
void RenderSystem::ClearScreen() {
    rlutil::cls();
    InitializePreviousFrame();
}

/// <summary>
/// ��������� ����
/// </summary>
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
                    rlutil::setColor(UnknownTileColor);
                    std::cout << UnknownTileChar;
                }

                m_previousFrame[y][x] = tileId;
                m_stats.tilesDrawn++; // ������� ������������ �����
            }
            else {
                m_stats.tilesSkipped++; // ������� ����������� �����
            }
        }
    }
    rlutil::setColor(15);
}

/// <summary>
/// ��������� ������
/// </summary>
void RenderSystem::DrawPlayer(int x, int y, int previousX, int previousY, const World& world) {
    // �������������� ����� �� ���������� ������� ������
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

    // ��������� ������ �� ����� �������
    if (x >= 0 && x < m_screenWidth && y >= 0 && y < m_screenHeight) {
        rlutil::locate(x, y);
        rlutil::setColor(PlayerColor);
        std::cout << PlayerChar;
        m_previousFrame[y][x] = PlayerTileId;
    }

    rlutil::setColor(15);
}

/// <summary>
/// ��������� ����������������� ����������
/// </summary>
void RenderSystem::DrawUI(const World& world, int posX, int posY) {
    rlutil::locate(0, m_screenHeight);
    for (int i = 0; i < m_screenWidth; i++) {
        std::cout << ' ';
    }

    rlutil::locate(0, m_screenHeight);
    std::cout << "Pos: " << posX << "," << posY;
    std::cout << " | Seed: " << world.GetCurrentSeed();
    std::cout << " | FPS: " << static_cast<int>(m_stats.currentFps);
    std::cout << " | Controls: WASD-move, Q-quit, R-regenerate";
}

/// <summary>
/// ������ ��������� �����
/// </summary>
void RenderSystem::StartFrame() {
    m_stats.frameStart = std::chrono::steady_clock::now();
    m_stats.tilesDrawn = 0;
    m_stats.tilesSkipped = 0;
}

/// <summary>
/// ���������� ��������� �����
/// </summary>
void RenderSystem::EndFrame() {
    auto frameEnd = std::chrono::steady_clock::now();
    auto frameTime = std::chrono::duration_cast<std::chrono::microseconds>(
        frameEnd - m_stats.frameStart).count();

    // ������ FPS
    if (frameTime > 0) {
        m_stats.currentFps = 1000000.0 / frameTime; // FPS = 1 / �����_�����_�_��������
    }
    else {
        m_stats.currentFps = 0.0;
    }

    m_stats.framesRendered++;

    // ���������� ���������� FPS ������ �������
    UpdateFPS();
}

/// <summary>
/// ���������� ���������� FPS
/// </summary>
void RenderSystem::UpdateFPS() {
    auto now = std::chrono::steady_clock::now();
    auto timeSinceLastUpdate = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - m_stats.lastFpsUpdate).count();

    if (timeSinceLastUpdate >= 1000) {
        m_stats.fpsHistory.push_back(m_stats.currentFps);

        if (m_stats.fpsHistory.size() > 60) {
            m_stats.fpsHistory.erase(m_stats.fpsHistory.begin());
        }

        double sum = 0.0;
        for (double fps : m_stats.fpsHistory) {
            sum += fps;
        }
        m_stats.averageFps = sum / m_stats.fpsHistory.size();

        m_stats.minFps = std::min(m_stats.minFps, m_stats.currentFps);
        m_stats.maxFps = std::max(m_stats.maxFps, m_stats.currentFps);

        m_stats.lastFpsUpdate = now;

        static int logCounter = 0;
        if (++logCounter >= 5) {
            LogStats();
            logCounter = 0;
        }
    }
}

/// <summary>
/// ����������� ����������
/// </summary>
void RenderSystem::LogStats() const {
    int totalTiles = m_screenWidth * m_screenHeight;
    double efficiency = (m_stats.tilesDrawn * 100.0) / totalTiles;

    Logger::Log("Render Stats - FPS: " + std::to_string(static_cast<int>(m_stats.currentFps)) +
        " | Avg: " + std::to_string(static_cast<int>(m_stats.averageFps)) +
        " | Min: " + std::to_string(static_cast<int>(m_stats.minFps)) +
        " | Max: " + std::to_string(static_cast<int>(m_stats.maxFps)) +
        " | Efficiency: " + std::to_string(static_cast<int>(efficiency)) + "%" +
        " | Tiles: " + std::to_string(m_stats.tilesDrawn) + "/" + std::to_string(totalTiles));
}