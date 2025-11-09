#pragma once
#include <memory>
#include <string>
#include <vector>
#include "InputManager.h"

class PauseMenu {
public:
    PauseMenu();
    void Initialize();
    void Update();
    void Render();
    void ProcessInput();

    bool ShouldResume() const { return m_shouldResume; }
    bool ShouldReturnToMainMenu() const { return m_shouldReturnToMainMenu; }
    void Reset();

private:
    void RenderMenu();
    void SelectNextOption();
    void SelectPreviousOption();
    void ConfirmSelection();

    std::unique_ptr<InputManager> m_inputManager;

    std::vector<std::string> m_menuOptions = {
        "Continue",
        "Return to Main Menu"
    };

    int m_selectedIndex;
    int m_prevSelectedIndex;
    bool m_shouldResume;
    bool m_shouldReturnToMainMenu;
    bool m_needRedraw;
};