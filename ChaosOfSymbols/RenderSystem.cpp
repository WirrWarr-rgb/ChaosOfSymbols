#include "RenderSystem.h"
#include "Logger.h"
#include <iostream>
#include <algorithm>
#define NOMINMAX
#include <windows.h>
#include <chrono>

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
    InitializePreviousFrame();

    // Инициализация статистики
    m_stats.lastFpsUpdate = std::chrono::steady_clock::now();
    m_stats.fpsHistory.reserve(60); // Храним историю за последние 60 кадров
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
    m_screenWidth = width;
    m_screenHeight = height;

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    COORD bufferSize = { static_cast<SHORT>(width), static_cast<SHORT>(height + 1) };
    SetConsoleScreenBufferSize(hConsole, bufferSize);

    SMALL_RECT windowSize = { 0, 0, static_cast<SHORT>(width - 1), static_cast<SHORT>(height + 1) };
    SetConsoleWindowInfo(hConsole, TRUE, &windowSize);

    InitializePreviousFrame();
    ClearScreen();
}

/// <summary>
/// Система двойной буферизации: перерисовка только изменившихся символов
/// </summary>
void RenderSystem::InitializePreviousFrame() {
    m_previousFrame.clear();
    m_previousFrame.resize(m_screenHeight, std::vector<int>(m_screenWidth, -1));
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
}

/// <summary>
/// Отрисовка мира С ГРАНИЦЕЙ
/// </summary>
void RenderSystem::DrawWorld(const World& world) {
    int totalWidth = world.GetTotalWidth();
    int totalHeight = world.GetTotalHeight();

    if (totalWidth != m_screenWidth || totalHeight != m_screenHeight) {
        SetScreenSize(totalWidth, totalHeight);
        ClearScreen();
    }

    // ОДИН ПРОХОД: сначала рисуем все статические объекты, потом динамические
    for (int y = 0; y < totalHeight; y++) {
        for (int x = 0; x < totalWidth; x++) {
            // Обработка границы
            if (x == 0 || x == totalWidth - 1 || y == 0 || y == totalHeight - 1) {
                if (NeedsRedraw(x, y, BORDER_TILE_ID)) {
                    rlutil::locate(x, y);
                    rlutil::setColor(15);
                    std::cout << '#';
                    m_previousFrame[y][x] = BORDER_TILE_ID;
                    m_stats.tilesDrawn++;
                }
                continue;
            }

            // Координаты в игровом пространстве
            int gameX = x - 1;
            int gameY = y - 1;

            // ПРОВЕРЯЕМ ЕДУ ПЕРВОЙ (динамический объект)
            const Food* food = world.GetFoodAt(gameX, gameY);
            if (food) {
                int foodId = FOOD_TILE_ID_BASE + food->GetId();
                if (NeedsRedraw(x, y, foodId)) {
                    rlutil::locate(x, y);
                    rlutil::setColor(food->GetColor());
                    std::cout << food->GetSymbol();
                    m_previousFrame[y][x] = foodId;
                    m_stats.tilesDrawn++;
                }
            }
            else {
                // ЕСЛИ ЕДЫ НЕТ - ОТРИСОВЫВАЕМ ТАЙЛ ЗЕМЛИ (статический объект)
                int tileId = world.GetTileAtFullMap(x, y);
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
                    m_stats.tilesDrawn++;
                }
            }
        }
    }
}

/// <summary>
/// Отрисовка игрока
/// </summary>
void RenderSystem::DrawPlayer(int x, int y, int previousX, int previousY, const World& world) {
    // Координаты игрока в игровом пространстве, преобразуем в координаты полной карты
    int screenX = x + 1;
    int screenY = y + 1;
    int prevScreenX = previousX + 1;
    int prevScreenY = previousY + 1;

    // ВОССТАНОВЛЕНИЕ ПРЕДЫДУЩЕЙ ПОЗИЦИИ ИГРОКА
    if (prevScreenX >= 0 && prevScreenX < m_screenWidth &&
        prevScreenY >= 0 && prevScreenY < m_screenHeight &&
        (prevScreenX != screenX || prevScreenY != screenY)) {

        // ВСЕГДА перерисовываем предыдущую позицию
        int gamePrevX = prevScreenX - 1;
        int gamePrevY = prevScreenY - 1;

        const Food* prevFood = world.GetFoodAt(gamePrevX, gamePrevY);
        if (prevFood) {
            // Если на предыдущей позиции была еда - восстанавливаем ее
            int foodId = FOOD_TILE_ID_BASE + prevFood->GetId();
            rlutil::locate(prevScreenX, prevScreenY);
            rlutil::setColor(prevFood->GetColor());
            std::cout << prevFood->GetSymbol();
            m_previousFrame[prevScreenY][prevScreenX] = foodId;
        }
        else {
            // Если еды не было - восстанавливаем тайл земли
            int tileId = world.GetTileAtFullMap(prevScreenX, prevScreenY);
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

    // ОТРИСОВКА ИГРОКА НА НОВОЙ ПОЗИЦИИ
    if (screenX >= 0 && screenX < m_screenWidth && screenY >= 0 && screenY < m_screenHeight) {
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
    int playerXP, int playerLevel, int xpToNextLevel) { // Добавьте эти параметры

    rlutil::locate(0, m_screenHeight);
    for (int i = 0; i < m_screenWidth; i++) {
        std::cout << ' ';
    }

    rlutil::locate(0, m_screenHeight);

    // ОТОБРАЖАЕМ ОПЫТ И УРОВЕНЬ
    std::cout << "Steps: " << playerSteps;
    std::cout << " | Lvl: " << playerLevel;
    std::cout << " | XP: " << playerXP << "/" << xpToNextLevel;
    std::cout << " | Health: " << playerHP << "/" << playerMaxHP;
    std::cout << " | Hunger: " << playerHunger << "/" << playerMaxHunger;
    std::cout << "\n";
    std::cout << "Pos: " << posX << "," << posY;
    std::cout << " | Seed: " << world.GetCurrentSeed();
    std::cout << " | FPS: " << static_cast<int>(m_stats.currentFps);
    std::cout << " | Controls: WASD-move, Q-quit";
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

    // Обновление статистики FPS каждую секунду
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