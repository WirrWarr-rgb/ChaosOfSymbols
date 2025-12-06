//#include "HelpPanel.h"
//#include <windows.h>
//#include <iostream>
//#include <algorithm>
//#include "rlutil.h"
//
//std::string HelpPanel::m_currentHelpText = "";
//std::string HelpPanel::m_currentItemId = "";
//
//HelpPanel::HelpPanel() {
//}
//
//void HelpPanel::Initialize() {
//    m_currentHelpText = "";
//    m_currentItemId = "";
//}
//
//void HelpPanel::Render(int screenWidth, int screenHeight) {
//    if (m_currentHelpText.empty()) {
//        return;
//    }
//
//    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
//
//    int panelY = screenHeight - PANEL_HEIGHT;
//
//    RenderBorder(screenWidth);
//
//    rlutil::locate(2, panelY + 1);
//    SetConsoleTextAttribute(hConsole, 7);
//
//    std::string displayText = m_currentHelpText;
//    int maxLength = screenWidth - 4;
//    if (displayText.length() > maxLength) {
//        displayText = displayText.substr(0, maxLength - 3) + "...";
//    }
//
//    std::cout << displayText;
//
//    SetConsoleTextAttribute(hConsole, 7);
//}
//
//void HelpPanel::RenderBorder(int screenWidth) {
//    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
//    int panelY = rlutil::trows() - PANEL_HEIGHT;
//
//    rlutil::locate(0, panelY);
//    SetConsoleTextAttribute(hConsole, 8);
//    for (int i = 0; i < screenWidth; i++) {
//        std::cout << "-";
//    }
//
//    rlutil::locate(0, panelY + PANEL_HEIGHT - 1);
//    for (int i = 0; i < screenWidth; i++) {
//        std::cout << "-";
//    }
//
//    SetConsoleTextAttribute(hConsole, 7);
//}
//
//void HelpPanel::RenderHelpText(int screenWidth) {
//}
//
//void HelpPanel::SetHelpText(const std::string& text) {
//    m_currentHelpText = text;
//}
//
//void HelpPanel::ClearHelpText() {
//    m_currentHelpText = "";
//    m_currentItemId = "";
//}
//
//void HelpPanel::SetCurrentItem(const std::string& itemId) {
//    m_currentItemId = itemId;
//    m_currentHelpText = HelpSystem::GetInstance().GetHelpText(itemId);
//}
//
//void HelpPanel::ResetCurrentItem() {
//    m_currentItemId = "";
//    m_currentHelpText = "";
//}