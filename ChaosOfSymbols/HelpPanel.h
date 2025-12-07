#ifndef HELPPANEL_H
#define HELPPANEL_H

#include <string>
#include <vector>

class HelpPanel {
public:
    static const int DEFAULT_PANEL_HEIGHT = 3;

    HelpPanel();
    static void Initialize();
    static void Render(int screenWidth, int screenHeight);
    static void SetHelpText(const std::string& text);
    static void ClearHelpText();
    static void SetCurrentItem(const std::string& itemId);
    static void ResetCurrentItem();

private:
    static void ClearPanelArea(int screenWidth, int screenHeight, int panelHeight);
    static void RenderBorder(int screenWidth, int panelHeight, int panelY);
    static void RenderHelpText(int screenWidth, int panelHeight, int panelY);
    static int CalculatePanelHeight(const std::string& text, int screenWidth);
    static std::vector<std::string> WrapText(const std::string& text, int maxLineLength);

    static std::string m_currentHelpText;
    static std::string m_previousHelpText;
    static std::string m_currentItemId;
    static int m_previousPanelHeight;
    static std::vector<std::string> m_cachedLines;
};

#endif // HELPPANEL_H