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
    , m_firstRender(true)
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
    if (m_firstRender) {
        rlutil::cls();
        m_firstRender = false;
        m_needRedraw = true;
    }

    if (m_needRedraw || NeedsRedraw()) {
        RenderOnlyChanges();
    }

    m_prevSelectedIndex = m_selectedIndex;
    m_needRedraw = false;
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
        m_shouldResume = true;
    }
}

bool PauseMenu::NeedsRedraw() const {
    return m_prevSelectedIndex != m_selectedIndex;
}

void PauseMenu::RenderOnlyChanges() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    int consoleWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    int consoleHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

    int centerX = consoleWidth / 2;
    int centerY = consoleHeight / 2;

    int menuHeight = m_menuOptions.size() + 4;
    int startY = centerY - menuHeight / 2;

    if (m_needRedraw) {
        SetConsoleTextAttribute(hConsole, 14);
        rlutil::locate(centerX - 10, startY);
        std::cout << "=== PAUSE MENU ===";
    }

    for (int i = 0; i < m_menuOptions.size(); ++i) {
        int line = startY + 2 + i;
        bool isSelected = (i == m_selectedIndex);
        bool wasSelected = (i == m_prevSelectedIndex);

        if (m_needRedraw || isSelected != wasSelected) {
            RenderMenuItem(line, m_menuOptions[i], isSelected);
        }
    }

    if (m_needRedraw) {
        rlutil::locate(centerX - 12, startY + m_menuOptions.size() + 2);
        SetConsoleTextAttribute(hConsole, 8);
        std::cout << "Press ESC to continue";
    }

    SetConsoleTextAttribute(hConsole, 7);
}

void PauseMenu::RenderMenuItem(int line, const std::string& text, bool selected) {
    ClearLine(line);
    rlutil::locate(0, line);

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    int consoleWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    int centerX = consoleWidth / 2;

    rlutil::locate(centerX - 8, line);

    if (selected) {
        SetConsoleTextAttribute(hConsole, 10);
        std::cout << "> " << text;
    }
    else {
        SetConsoleTextAttribute(hConsole, 7);
        std::cout << "  " << text;
    }
}

void PauseMenu::ClearLine(int line) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    int consoleWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;

    rlutil::locate(0, line);
    for (int i = 0; i < consoleWidth; i++) {
        std::cout << ' ';
    }
}

void PauseMenu::SelectNextOption() {
    m_selectedIndex = (m_selectedIndex + 1) % m_menuOptions.size();
}

void PauseMenu::SelectPreviousOption() {
    m_selectedIndex = (m_selectedIndex - 1 + m_menuOptions.size()) % m_menuOptions.size();
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
    m_firstRender = true;

    if (m_inputManager) {
        m_inputManager->ClearSystemBuffer();
        m_inputManager->ClearState();
    }
}