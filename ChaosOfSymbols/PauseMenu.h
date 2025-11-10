#pragma once
#include <memory>
#include <vector>
#include <string>
#include "InputManager.h"

class PauseMenu {
private:
    std::vector<std::string> m_menuOptions = { "Continue", "Return to Main Menu" };
    std::unique_ptr<InputManager> m_inputManager;

    int m_selectedIndex;
    int m_prevSelectedIndex;

    bool m_shouldResume;
    bool m_shouldReturnToMainMenu;
    bool m_needRedraw;
    bool m_firstRender;

public:
    PauseMenu();

    void Initialize();
    void Update();
    void Render();
    void ProcessInput();
    void Reset();

    bool ShouldResume() const { return m_shouldResume; }
    bool ShouldReturnToMainMenu() const { return m_shouldReturnToMainMenu; }

private:
    void RenderMenu();
    void RenderOnlyChanges();
    bool NeedsRedraw() const;
    void RenderMenuItem(int line, const std::string& text, bool selected);
    void SelectNextOption();
    void SelectPreviousOption();
    void ConfirmSelection();
    void ClearLine(int line);
};