#include <iostream>
#include <algorithm>
#include <sstream>
#define NOMINMAX
#include <windows.h>
#include <chrono>
#include "RenderSystem.h"
#include "Logger.h"

namespace rlutil {
    /// <summary>
    /// Установка цвета текста
    /// </summary>
    /// <param name="color">Цвет</param>
    void setColor(int color) {
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
    }

    /// <summary>
    /// Очистка экрана
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
    /// Перемещение курсора в координаты X.Y.
    /// </summary>
    /// <param name="x">Координата X</param>
    /// <param name="y">Координата Y</param>
    void locate(int x, int y) {
        COORD coord;
        coord.X = static_cast<SHORT>(x);
        coord.Y = static_cast<SHORT>(y);
        SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
    }

    /// <summary>
    /// Скрытие курсора
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
/// Конструктор: инициализация
/// </summary>
RenderSystem::RenderSystem(TileTypeManager* tileManager)
    : m_tileManager(tileManager) {
    rlutil::hideCursor();
    m_screenWidth = DefaultScreenWidth;
    m_screenHeight = DefaultScreenHeight;

    UpdateScreenSizeFromConsole();
    InitializePreviousFrame();

    m_previousUILines.resize(UI_LINES, "");

    m_stats.lastFpsUpdate = std::chrono::steady_clock::now();
    m_stats.fpsHistory.reserve(60);
}

/// <summary>
/// Деструктор: восстановление консоли
/// </summary>
RenderSystem::~RenderSystem() {
    COORD size = { DefaultScreenWidth, DefaultScreenHeight - 1 };
    SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), size);

    SMALL_RECT rect = { 0, 0, DefaultScreenWidth - 1, DefaultScreenHeight - 1 };
    SetConsoleWindowInfo(GetStdHandle(STD_OUTPUT_HANDLE), TRUE, &rect);
}

/// <summary>
/// Управление размером экрана
/// </summary>
/// <param name="width">Ширина</param>
/// <param name="height">Высота</param>
void RenderSystem::SetScreenSize(int width, int height) {

    const int MIN_SCREEN_WIDTH = 70;

    m_screenWidth = std::max(width, MIN_SCREEN_WIDTH);

    m_screenHeight = height + UI_LINES;

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    // Простая установка через системную команду
    std::string command = "mode con: cols=" +
        std::to_string(m_screenWidth) + " lines=" +
        std::to_string(m_screenHeight);
    system(command.c_str());

    COORD bufferSize = {
        static_cast<SHORT>(m_screenWidth),
        static_cast<SHORT>(m_screenHeight)
    };
    SetConsoleScreenBufferSize(hConsole, bufferSize);

    SMALL_RECT windowSize = {
        0,
        0,
        static_cast<SHORT>(m_screenWidth - 1),
        static_cast<SHORT>(m_screenHeight - 1)
    };
    SetConsoleWindowInfo(hConsole, TRUE, &windowSize);

    ClearEntireScreen();

    UpdateScreenSizeFromConsole();

    Logger::Log("Screen size set to: " + std::to_string(m_screenWidth) + "x" +
        std::to_string(m_screenHeight) + " (world: " + std::to_string(width) + "x" +
        std::to_string(height) + " + UI)");
}


/// <summary>
/// Система двойной буферизации: перерисовка только изменившихся символов
/// </summary>
void RenderSystem::InitializePreviousFrame() {
    m_previousFrame.clear();
    m_previousFrame.resize(m_screenHeight, std::vector<int>(m_screenWidth, -1));

    m_previousUILines.clear();
    m_previousUILines.resize(UI_LINES, "");
    m_uiNeedsRedraw = true;
}

/// <summary>
/// Система двойной буферизации: проверка изменился ли тайл
/// </summary>
bool RenderSystem::NeedsRedraw(int x, int y, int tileId) {
    if (x < 0 || x >= m_screenWidth || y < 0 || y >= m_screenHeight)
        return false;

    return m_previousFrame[y][x] != tileId;
}

/// <summary>
/// Очистка консоли
/// </summary>
void RenderSystem::ClearScreen() {
    rlutil::cls();
    InitializePreviousFrame();
    m_uiNeedsRedraw = true;
}

/// <summary>
/// Отрисовка мира С ГРАНИЦЕЙ
/// </summary>
void RenderSystem::DrawWorld(const World& world) {
    rlutil::hideCursor();
    HandleConsoleResize();

    int totalWidth = world.GetTotalWidth();
    int totalHeight = world.GetTotalHeight();

    int viewportWidth = std::min(totalWidth, m_screenWidth);
    int viewportHeight = std::min(totalHeight, m_screenHeight);

    if (viewportWidth != m_screenWidth || m_forceRedraw) {
        m_screenWidth = viewportWidth;
        InitializePreviousFrame();
        m_forceRedraw = false;
    }

    for (int y = 0; y < viewportHeight; y++) {
        for (int x = 0; x < viewportWidth; x++) {
            if (x == 0 || x == totalWidth - 1 || y == 0 || y == totalHeight - 1) {
                if (x < viewportWidth && y < viewportHeight) {
                    if (NeedsRedraw(x, y, BORDER_TILE_ID) || m_forceRedraw) {
                        rlutil::locate(x, y);
                        rlutil::setColor(15);
                        std::cout << '#';
                        m_previousFrame[y][x] = BORDER_TILE_ID;
                        m_stats.tilesDrawn++;
                    }
                }
                continue;
            }

            int gameX = x - 1;
            int gameY = y - 1;

            if (gameX >= 0 && gameX < world.GetWidth() &&
                gameY >= 0 && gameY < world.GetHeight()) {

                const Food* food = world.GetFoodAt(gameX, gameY);
                if (food) {
                    int foodId = FOOD_TILE_ID_BASE + food->GetId();
                    if (NeedsRedraw(x, y, foodId) || m_forceRedraw) {
                        rlutil::locate(x, y);
                        rlutil::setColor(food->GetColor());
                        std::cout << food->GetSymbol();
                        m_previousFrame[y][x] = foodId;
                        m_stats.tilesDrawn++;
                    }
                }
                else {
                    int tileId = world.GetTileAt(gameX, gameY);
                    if (NeedsRedraw(x, y, tileId) || m_forceRedraw) {
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
                        m_stats.tilesDrawn++;
                    }
                }
            }
        }
    }
}

/// <summary>
/// Отрисовка игрока
/// </summary>
void RenderSystem::DrawPlayer(int x, int y, int previousX, int previousY, const World& world) {
    int screenX = x + 1;
    int screenY = y + 1;
    int prevScreenX = previousX + 1;
    int prevScreenY = previousY + 1;

    bool currentInViewport = (screenX >= 0 && screenX < m_screenWidth &&
        screenY >= 0 && screenY < m_screenHeight);
    bool prevInViewport = (prevScreenX >= 0 && prevScreenX < m_screenWidth &&
        prevScreenY >= 0 && prevScreenY < m_screenHeight);

    if (prevInViewport && (prevScreenX != screenX || prevScreenY != screenY)) {
        int gamePrevX = prevScreenX - 1;
        int gamePrevY = prevScreenY - 1;

        if (gamePrevX >= 0 && gamePrevX < world.GetWidth() &&
            gamePrevY >= 0 && gamePrevY < world.GetHeight()) {

            const Food* prevFood = world.GetFoodAt(gamePrevX, gamePrevY);
            if (prevFood) {
                int foodId = FOOD_TILE_ID_BASE + prevFood->GetId();
                rlutil::locate(prevScreenX, prevScreenY);
                rlutil::setColor(prevFood->GetColor());
                std::cout << prevFood->GetSymbol();
                m_previousFrame[prevScreenY][prevScreenX] = foodId;
            }
            else {
                int tileId = world.GetTileAt(gamePrevX, gamePrevY);
                rlutil::locate(prevScreenX, prevScreenY);
                TileType* tile = m_tileManager->GetTileType(tileId);
                if (tile) {
                    rlutil::setColor(tile->GetColor());
                    std::cout << tile->GetCharacter();
                }
                m_previousFrame[prevScreenY][prevScreenX] = tileId;
            }
            m_stats.tilesDrawn++;
        }
    }

    if (currentInViewport) {
        rlutil::locate(screenX, screenY);
        rlutil::setColor(PlayerColor);
        std::cout << PlayerChar;
        m_previousFrame[screenY][screenX] = PLAYER_TILE_ID;
        m_stats.tilesDrawn++;
    }

    rlutil::setColor(15);
}

/// <summary>
/// Отрисовка пользовательского интерфейса
/// </summary>
void RenderSystem::DrawUI(const World& world, int posX, int posY, int playerSteps,
    int playerHP, int playerMaxHP, int playerHunger, int playerMaxHunger,
    int playerXP, int playerLevel, int xpToNextLevel) {

    int worldHeight = world.GetTotalHeight();
    int uiStartLine = worldHeight;

    if (uiStartLine >= m_screenHeight) {
        uiStartLine = m_screenHeight - UI_LINES;
    }

    std::vector<std::string> newUILines(UI_LINES);

    std::stringstream uiStream;
    uiStream << "Steps: " << playerSteps;
    uiStream << " | Lvl: " << playerLevel;
    uiStream << " | XP: " << playerXP << "/" << xpToNextLevel;
    uiStream << " | Health: " << playerHP << "/" << playerMaxHP;
    uiStream << " | Hunger: " << playerHunger << "/" << playerMaxHunger;

    newUILines[0] = uiStream.str();


    std::stringstream infoStream;
    infoStream << "Pos: " << posX << "," << posY;
    infoStream << " | Seed: " << world.GetCurrentSeed();
    infoStream << " | FPS: " << static_cast<int>(m_stats.currentFps);

    newUILines[1] = infoStream.str();

    bool uiChanged = false;
    for (int i = 0; i < UI_LINES; i++) {
        if (m_previousUILines[i] != newUILines[i]) {
            uiChanged = true;
            break;
        }
    }

    if (uiChanged || m_uiNeedsRedraw || m_forceRedraw) {
        ClearUIArea(uiStartLine);

        for (int i = 0; i < UI_LINES; i++) {
            if (uiStartLine + i < m_screenHeight) {
                rlutil::locate(0, uiStartLine + i);
                std::cout << newUILines[i];

                int spacesNeeded = m_screenWidth - newUILines[i].length();
                if (spacesNeeded > 0) {
                    std::cout << std::string(spacesNeeded, ' ');
                }
            }
        }

        m_previousUILines = newUILines;
        m_uiNeedsRedraw = false;

        Logger::Log("UI redrawn - Steps: " + std::to_string(playerSteps) +
            ", HP: " + std::to_string(playerHP) +
            ", FPS: " + std::to_string(static_cast<int>(m_stats.currentFps)));
    }
}

/// <summary>
/// Полная очистка области UI
/// </summary>
void RenderSystem::ClearUIArea(int uiStartLine) {
    for (int line = uiStartLine; line < m_screenHeight; line++) {
        rlutil::locate(0, line);
        for (int i = 0; i < m_screenWidth; i++) {
            std::cout << ' ';
        }
    }
}


/// <summary>
/// Начало отрисовки кадра
/// </summary>
void RenderSystem::StartFrame() {
    m_stats.frameStart = std::chrono::steady_clock::now();
    m_stats.tilesDrawn = 0;
    m_stats.tilesSkipped = 0;
}

/// <summary>
/// Завершение отрисовки кадра
/// </summary>
void RenderSystem::EndFrame() {
    auto frameEnd = std::chrono::steady_clock::now();
    auto frameTime = std::chrono::duration_cast<std::chrono::microseconds>(
        frameEnd - m_stats.frameStart).count();

    // Расчет FPS
    if (frameTime > 0) {
        m_stats.currentFps = 1000000.0 / frameTime; // FPS = 1 / время_кадра_в_секундах
    }
    else {
        m_stats.currentFps = 0.0;
    }

    m_stats.framesRendered++;

    UpdateFPS();
}

/// <summary>
/// Обновление статистики FPS
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
/// Логирование статистики
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

/// <summary>
/// Очистка экрана
/// </summary>
void RenderSystem::ClearEntireScreen() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);

    COORD topLeft = { 0, 0 };
    DWORD consoleSize = csbi.dwSize.X * csbi.dwSize.Y;
    DWORD written;

    FillConsoleOutputCharacterA(hConsole, ' ', consoleSize, topLeft, &written);
    FillConsoleOutputAttribute(hConsole, 7, consoleSize, topLeft, &written);
    SetConsoleCursorPosition(hConsole, topLeft);

    rlutil::cls();

    InitializePreviousFrame();

    m_uiNeedsRedraw = true;

    rlutil::hideCursor();
}

void RenderSystem::HandleConsoleResize() {
    if (HasConsoleSizeChanged()) {
        Logger::Log("Console size changed - adapting...");
        rlutil::hideCursor();

        AdaptToConsoleSize();
    }
}

bool RenderSystem::HasConsoleSizeChanged() const {
    int currentWidth = rlutil::tcols();
    int currentHeight = rlutil::trows();

    return currentWidth != m_actualConsoleWidth || currentHeight != m_actualConsoleHeight;
}

void RenderSystem::UpdateScreenSizeFromConsole() {
    m_actualConsoleWidth = rlutil::tcols();
    m_actualConsoleHeight = rlutil::trows();
}

void RenderSystem::AdaptToConsoleSize() {
    int newWidth = rlutil::tcols();
    int newHeight = rlutil::trows();

    m_actualConsoleWidth = newWidth;
    m_actualConsoleHeight = newHeight;

    if (m_screenWidth != newWidth || m_screenHeight != newHeight) {
        m_screenWidth = newWidth;
        m_screenHeight = newHeight;

        ClearEntireScreen();
        m_forceRedraw = true;
        m_uiNeedsRedraw = true;

        Logger::Log("Console size changed - full redraw: " +
            std::to_string(m_screenWidth) + "x" + std::to_string(m_screenHeight));
    }
}