#include "HelpPanel.h"
#include "HelpSystem.h"
#include <windows.h>
#include <iostream>
#include <algorithm>
#include <vector>
#include <sstream>

std::string HelpPanel::m_currentHelpText = "";
std::string HelpPanel::m_previousHelpText = "";
std::string HelpPanel::m_currentItemId = "";
int HelpPanel::m_previousPanelHeight = 0;
std::vector<std::string> HelpPanel::m_cachedLines;

HelpPanel::HelpPanel() {
}

void HelpPanel::Initialize() {
    m_currentHelpText = "";
    m_previousHelpText = "";
    m_currentItemId = "";
    m_previousPanelHeight = 0;
    m_cachedLines.clear();
}

void HelpPanel::Render(int screenWidth, int screenHeight) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    int windowHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

    if (m_currentHelpText == m_previousHelpText && m_currentHelpText.empty()) {
        return;
    }

    if (m_currentHelpText.empty()) {
        if (m_previousPanelHeight > 0) {
            ClearPanelArea(screenWidth, windowHeight, m_previousPanelHeight);
            m_previousPanelHeight = 0;
            m_previousHelpText = "";
            m_cachedLines.clear();
        }
        return;
    }

    int panelHeight = CalculatePanelHeight(m_currentHelpText, screenWidth);

    if (panelHeight > windowHeight - 2) {
        panelHeight = windowHeight - 2;
    }

    bool needsRedraw = false;

    if (m_currentHelpText != m_previousHelpText) {
        needsRedraw = true;

        int maxLineLength = screenWidth - 4;
        m_cachedLines = WrapText(m_currentHelpText, maxLineLength);

        int maxLines = panelHeight - 2;
        if (m_cachedLines.size() > maxLines) {
            m_cachedLines.resize(maxLines);
            if (!m_cachedLines.empty() && m_cachedLines.back().length() + 3 <= maxLineLength) {
                m_cachedLines.back() += "...";
            }
        }
    }

    if (panelHeight != m_previousPanelHeight) {
        needsRedraw = true;
        if (m_previousPanelHeight > 0) {
            ClearPanelArea(screenWidth, windowHeight, m_previousPanelHeight);
        }
    }

    if (!needsRedraw && m_previousPanelHeight > 0) {
        return;
    }

    int panelY = windowHeight - panelHeight;

    COORD originalCursorPos = csbi.dwCursorPosition;

    ClearPanelArea(screenWidth, windowHeight, panelHeight);

    RenderBorder(screenWidth, panelHeight, panelY);

    RenderHelpText(screenWidth, panelHeight, panelY);

    SetConsoleCursorPosition(hConsole, originalCursorPos);

    m_previousHelpText = m_currentHelpText;
    m_previousPanelHeight = panelHeight;
}

void HelpPanel::ClearPanelArea(int screenWidth, int windowHeight, int panelHeight) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    int panelY = windowHeight - panelHeight;

    for (int y = panelY; y < windowHeight; y++) {
        COORD clearCoord = { 0, (SHORT)y };
        SetConsoleCursorPosition(hConsole, clearCoord);

        for (int i = 0; i < screenWidth; i++) {
            std::cout << ' ';
        }
    }
}

void HelpPanel::RenderBorder(int screenWidth, int panelHeight, int panelY) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    COORD borderCoord = { 0, (SHORT)panelY };
    SetConsoleCursorPosition(hConsole, borderCoord);
    SetConsoleTextAttribute(hConsole, 8);
    for (int i = 0; i < screenWidth; i++) {
        std::cout << "-";
    }

    if (panelHeight > 1) {
        borderCoord.Y = panelY + panelHeight - 1;
        SetConsoleCursorPosition(hConsole, borderCoord);
        for (int i = 0; i < screenWidth; i++) {
            std::cout << "-";
        }
    }

    SetConsoleTextAttribute(hConsole, 7);
}

void HelpPanel::RenderHelpText(int screenWidth, int panelHeight, int panelY) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    SetConsoleTextAttribute(hConsole, 7);

    if (m_cachedLines.empty()) {
        int maxLineLength = screenWidth - 4;
        m_cachedLines = WrapText(m_currentHelpText, maxLineLength);

        int maxLines = panelHeight - 2;
        if (m_cachedLines.size() > maxLines) {
            m_cachedLines.resize(maxLines);
            if (!m_cachedLines.empty() && m_cachedLines.back().length() + 3 <= maxLineLength) {
                m_cachedLines.back() += "...";
            }
        }
    }

    for (size_t i = 0; i < m_cachedLines.size(); i++) {
        if (panelY + 1 + static_cast<int>(i) >= 25) {
            break;
        }

        COORD textCoord = { 2, (SHORT)(panelY + 1 + i) };
        SetConsoleCursorPosition(hConsole, textCoord);
        std::cout << m_cachedLines[i];

        int maxLineLength = screenWidth - 4;
        for (size_t j = m_cachedLines[i].length(); j < static_cast<size_t>(maxLineLength); j++) {
            std::cout << ' ';
        }
    }
}

int HelpPanel::CalculatePanelHeight(const std::string& text, int screenWidth) {
    if (text.empty()) {
        return 0;
    }

    int maxLineLength = screenWidth - 4;
    std::vector<std::string> lines = WrapText(text, maxLineLength);

    return 2 + static_cast<int>(lines.size());
}

std::vector<std::string> HelpPanel::WrapText(const std::string& text, int maxLineLength) {
    std::vector<std::string> lines;
    std::string currentLine;
    std::istringstream wordsStream(text);
    std::string word;

    while (wordsStream >> word) {
        if (currentLine.length() + word.length() + 1 > maxLineLength) {
            if (!currentLine.empty()) {
                lines.push_back(currentLine);
                currentLine.clear();
            }

            if (word.length() > maxLineLength) {
                for (size_t i = 0; i < word.length(); i += maxLineLength) {
                    std::string part = word.substr(i, maxLineLength);
                    lines.push_back(part);
                }
                continue;
            }
        }

        if (!currentLine.empty()) {
            currentLine += " ";
        }
        currentLine += word;
    }

    if (!currentLine.empty()) {
        lines.push_back(currentLine);
    }

    return lines;
}


void HelpPanel::SetHelpText(const std::string& text) {
    if (text != m_currentHelpText) {
        m_currentHelpText = text;
    }
}

void HelpPanel::ClearHelpText() {
    m_currentHelpText = "";
    m_currentItemId = "";
}

void HelpPanel::SetCurrentItem(const std::string& itemId) {
    m_currentItemId = itemId;
    std::string newHelpText = HelpSystem::GetInstance().GetHelpText(itemId);

    if (newHelpText != m_currentHelpText) {
        m_currentHelpText = newHelpText;
    }
}

void HelpPanel::ResetCurrentItem() {
    m_currentItemId = "";
    m_currentHelpText = "";
}