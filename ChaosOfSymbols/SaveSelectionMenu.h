#pragma once
#include <vector>
#include <string>
#include <memory>
#include "SaveTypes.h"
#include "SaveSystem.h"
#include "InputManager.h"
#include "WorldEditor.h"
#include "TemplateSystem.h"

class WorldEditor;

enum class SaveActionState {
    MAIN_LIST,
    WORLD_EDITOR,
    TEMPLATE_SELECTION
};

class SaveSelectionMenu {
public:
    SaveSelectionMenu();

    void Initialize();
    void Update();
    void Render();
    void Reset();
    void ProcessInput();

    bool ShouldReturnToMainMenu() const { return m_shouldReturn; }
    bool ShouldStartGame() const { return m_shouldStartGame; }
    int GetSelectedSlot() const { return m_selectedSlot; }
    GameMode GetGameMode() const { return m_gameMode; }

private:
    void RenderOnlyChanges();
    void RenderSavesList();
    void RenderTemplatesList();
    void SelectNextOption();
    void SelectPreviousOption();
    void ConfirmSelection();
    void ClearMenuArea();
    void ClearLine(int line);
    void RenderSaveItem(int line, const SaveInfo& save, bool selected);
    void RenderTemplateItem(int line, const TemplateInfo& templateInfo, bool selected);
    void RenderActionItem(int line, const std::string& text, bool selected);
    bool NeedsRedraw() const;
    void ShowCreateOptionsForSlot(int slot);

    void UpdateHelpForCurrentSelection();

    void ShowTemplatesForSlot(int slot);
    void SelectTemplateForSave(int templateSlot);

    int m_prevSelectedTemplateIndex;
    bool m_prevBackSelected;

    GameMode m_gameMode;
    std::vector<SaveInfo> m_saves;
    std::vector<TemplateInfo> m_templates;
    std::vector<std::string> m_emptySaveActions;
    std::vector<std::string> m_usedSaveActions;
    int m_selectedSlot;
    int m_selectedActionIndex;
    bool m_shouldReturn;
    bool m_shouldStartGame;
    std::unique_ptr<SaveSystem> m_saveSystem;
    std::unique_ptr<TemplateSystem> m_templateSystem;

    std::unique_ptr<WorldEditor> m_worldEditor;

    int m_prevSelectedSlot;
    int m_prevSelectedActionIndex;
    SaveActionState m_currentState;
    SaveActionState m_prevState;
    std::vector<SaveInfo> m_prevSaves;
    bool m_needFullRedraw;
    bool m_ignoreFirstInput;
    std::unique_ptr<InputManager> m_inputManager;

    bool m_showActionsForSlot;
    int m_actionSlot;

    int m_selectedTemplateIndex;
    int m_templateForSlot;
    bool m_inTemplateSelection;
};