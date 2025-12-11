#pragma once
#include <vector>
#include <string>
#include <functional>
#include <memory>
#include "SaveTypes.h"
#include "SaveSelectionMenu.h"
#include "WorldTemplateMenu.h"
#include "InputManager.h"

enum class MainMenuOption {
    PLAY_PROCEDURAL,
    PLAY_PRELOADED,
    PLAY_BACK,
    ABOUT,
    EXIT
};

enum class MenuState {
    MAIN_MENU,
    ABOUT_SCREEN,
    SAVE_SELECTION,
    WORLD_TEMPLATE_MENU,
    IN_GAME
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

    GameMode GetSelectedSaveGameMode() const { return m_selectedSaveGameMode; }
    int GetSelectedSaveSlot() const { return m_selectedSaveSlot; }
    bool ShouldLoadSave() const { return m_shouldLoadSave; }
    void ResetExitFlag() { m_shouldExit = false; }

    void Reset() {
        m_shouldStartGame = false;
        m_shouldExit = false;
        m_shouldLoadSave = false;
        m_currentState = MenuState::MAIN_MENU;
        m_inPlaySubmenu = false;
        m_selectedMainIndex = 0;
        m_selectedSubIndex = 0;
        m_needFullRedraw = true;

        m_saveSelectionMenu.reset();
        m_worldTemplateMenu.reset();
    }

    void ResetStartFlags() {
        m_shouldStartGame = false;
        m_shouldLoadSave = false;
    }

private:
    void RenderMainMenu();
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

    void InitializeSaveSelection();
    void InitializeWorldTemplateMenu();
    void RunSaveSelection();
    void RunWorldTemplateMenu();

    void UpdateHelpForCurrentSelection();
    void RegisterMenuHelpEntries();

private:
    MenuState m_currentState;
    std::vector<std::string> m_mainMenuOptions;
    std::vector<std::string> m_playSubOptions;
    int m_selectedMainIndex;
    int m_selectedSubIndex;
    bool m_inPlaySubmenu;
    bool m_shouldStartGame;
    bool m_shouldExit;
    bool m_shouldLoadSave;
    MainMenuOption m_selectedGameMode;

    std::unique_ptr<SaveSelectionMenu> m_saveSelectionMenu;
    bool m_inSaveSelection;
    GameMode m_selectedSaveGameMode;
    int m_selectedSaveSlot;

    std::unique_ptr<WorldTemplateMenu> m_worldTemplateMenu;
    bool m_inTemplateSelection;

    int m_prevSelectedMainIndex;
    int m_prevSelectedSubIndex;
    bool m_prevInPlaySubmenu;
    MenuState m_prevState;
    bool m_needFullRedraw;

    std::unique_ptr<InputManager> m_inputManager;
};