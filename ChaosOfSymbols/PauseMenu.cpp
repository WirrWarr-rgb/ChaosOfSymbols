#include "PauseMenu.h"
#include <windows.h>
#include <iostream>
#include "Logger.h"

namespace rlutil {
    void setColor(int color);
    void cls();
    void locate(int x, int y);
    void hideCursor();
}

PauseMenu::PauseMenu()
    : m_selectedIndex(0)
    , m_prevSelectedIndex(-1)
    , m_shouldResume(false)
    , m_shouldReturnToMainMenu(false)
    , m_needRedraw(true)
{
    m_inputManager = std::make_unique<InputManager>();
}

void PauseMenu::Initialize() {
    rlutil::hideCursor();
    if (m_inputManager) {
        m_inputManager->ClearSystemBuffer();
        m_inputManager->ClearState();
    }
}

void PauseMenu::Update() {
    if (m_inputManager) {
        m_inputManager->Update();
    }
}

void PauseMenu::Render() {
    RenderMenu();
}

void PauseMenu::ProcessInput() {
    if (!m_inputManager) return;

    if (m_inputManager->IsMenuUp()) {
        SelectPreviousOption();
    }
    else if (m_inputManager->IsMenuDown()) {
        SelectNextOption();
    }
    else if (m_inputManager->IsMenuSelect()) {
        ConfirmSelection();
    }
    else if (m_inputManager->IsMenuBack()) {
        // ESC также продолжает игру
        m_shouldResume = true;
    }
}

void PauseMenu::RenderMenu() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    // Получаем размеры консоли для центрирования
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    int consoleWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    int consoleHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

    // Центрируем меню
    int centerX = consoleWidth / 2;
    int centerY = consoleHeight / 2;

    int menuWidth = 20;
    int menuHeight = m_menuOptions.size() + 4;

    int startX = centerX - menuWidth / 2;
    int startY = centerY - menuHeight / 2;

    // Очищаем область меню если нужно
    if (m_needRedraw) {
        for (int y = startY; y < startY + menuHeight; y++) {
            rlutil::locate(startX, y);
            for (int x = 0; x < menuWidth; x++) {
                std::cout << ' ';
            }
        }
    }

    // Заголовок
    rlutil::locate(centerX - 5, startY);
    SetConsoleTextAttribute(hConsole, 14); // Желтый
    std::cout << "PAUSE MENU";

    // Опции меню
    for (int i = 0; i < m_menuOptions.size(); ++i) {
        rlutil::locate(centerX - 6, startY + 2 + i);

        bool isSelected = (i == m_selectedIndex);
        if (isSelected) {
            SetConsoleTextAttribute(hConsole, 10); // Зеленый
            std::cout << "> " << m_menuOptions[i];
        }
        else {
            SetConsoleTextAttribute(hConsole, 7); // Белый
            std::cout << "  " << m_menuOptions[i];
        }
    }

    // Инструкции
    rlutil::locate(centerX - 10, startY + m_menuOptions.size() + 2);
    SetConsoleTextAttribute(hConsole, 8); // Серый
    std::cout << "Press ESC to continue";

    SetConsoleTextAttribute(hConsole, 7); // Возвращаем белый цвет

    m_prevSelectedIndex = m_selectedIndex;
    m_needRedraw = false;
}

void PauseMenu::SelectNextOption() {
    m_selectedIndex = (m_selectedIndex + 1) % m_menuOptions.size();
    m_needRedraw = true;
}

void PauseMenu::SelectPreviousOption() {
    m_selectedIndex = (m_selectedIndex - 1 + m_menuOptions.size()) % m_menuOptions.size();
    m_needRedraw = true;
}

void PauseMenu::ConfirmSelection() {
    switch (m_selectedIndex) {
    case 0: // Continue
        m_shouldResume = true;
        break;
    case 1: // Return to Main Menu
        m_shouldReturnToMainMenu = true;
        break;
    }
}

void PauseMenu::Reset() {
    m_selectedIndex = 0;
    m_prevSelectedIndex = -1;
    m_shouldResume = false;
    m_shouldReturnToMainMenu = false;
    m_needRedraw = true;

    if (m_inputManager) {
        m_inputManager->ClearSystemBuffer();
        m_inputManager->ClearState();
    }
}