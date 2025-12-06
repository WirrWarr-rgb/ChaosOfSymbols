//#ifndef HELPPANEL_H
//#define HELPPANEL_H
//
//#include <string>
//#include <functional>
//#include "HelpSystem.h"
//
//class HelpPanel {
//public:
//    HelpPanel();
//
//    static void Initialize();
//
//    static void Render(int screenWidth = 80, int screenHeight = 25);
//
//    static void SetHelpText(const std::string& text);
//
//    static void ClearHelpText();
//
//    static void SetCurrentItem(const std::string& itemId);
//
//    static void ResetCurrentItem();
//
//    static int GetPanelHeight() { return PANEL_HEIGHT; }
//
//private:
//    static const int PANEL_HEIGHT = 3;
//    static std::string m_currentHelpText;
//    static std::string m_currentItemId;
//
//    static void RenderBorder(int screenWidth);
//    static void RenderHelpText(int screenWidth);
//};
//
//#endif // HELPPANEL_H