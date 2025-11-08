#pragma once
#include <vector>
#include <string>
#include <functional>

enum class MenuState {
    MAIN_MENU,
    ABOUT_SCREEN,
    IN_GAME
};

enum class MainMenuOption {
    PLAY_PROCEDURAL,
    PLAY_PRELOADED,
    PLAY_BACK,
    ABOUT,
    EXIT
};

class MainMenu {
public:
    MainMenu();
    void Initialize();
    void Update();
    void Render();
    void ProcessInput();

    bool ShouldStartGame() const { return m_shouldStartGame; }
    bool ShouldExitGame() const { return m_shouldExit; }
    MenuState GetCurrentState() const { return m_currentState; }
    MainMenuOption GetSelectedGameMode() const { return m_selectedGameMode; }
    void Reset() {
        m_shouldStartGame = false;
        m_shouldExit = false;
        m_currentState = MenuState::MAIN_MENU;
        m_inPlaySubmenu = false;
        m_selectedMainIndex = 0;
        m_selectedSubIndex = 0;
        m_needFullRedraw = true;
    }

private:
    void RenderAboutScreen();
    void RenderOnlyChanges();
    void SelectNextOption();
    void SelectPreviousOption();
    void ConfirmSelection();
    void ClearMenuArea();
    void ClearLine(int line);
    bool NeedsRedraw() const;
    void RenderMenuItem(int index, int line, const std::string& text, bool selected);
    void RenderSubMenuItem(int index, int line, const std::string& text, bool selected);

private:
    MenuState m_currentState;
    std::vector<std::string> m_mainMenuOptions;
    std::vector<std::string> m_playSubOptions;
    int m_selectedMainIndex;
    int m_selectedSubIndex;
    bool m_inPlaySubmenu;
    bool m_shouldStartGame;
    bool m_shouldExit;
    MainMenuOption m_selectedGameMode;
    int m_prevSelectedMainIndex;
    int m_prevSelectedSubIndex;
    bool m_prevInPlaySubmenu;
    MenuState m_prevState;
    bool m_needFullRedraw;
};