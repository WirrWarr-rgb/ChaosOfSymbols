#pragma once
#include <vector>
#include <string>
#include <memory>
#include "InputManager.h"
#include "WorldEditor.h"
#include "TemplateSystem.h"

enum class TemplateActionState {
    MAIN_LIST,
    WORLD_EDITOR
};

class WorldTemplateMenu {
public:
    WorldTemplateMenu();
    void Initialize();
    void Update();
    void Render();
    void ProcessInput();
    void Reset();

    bool ShouldReturnToMainMenu() const { return m_shouldReturn; }
    bool ShouldCreateTemplate() const { return m_shouldCreateTemplate; }
    int GetSelectedSlot() const { return m_selectedSlot; }

private:
    void RenderOnlyChanges();
    void RenderTemplatesList();
    void SelectNextOption();
    void SelectPreviousOption();
    void SelectLeftOption();
    void SelectRightOption();
    void ConfirmSelection();
    void ClearLine(int line);
    void RenderTemplateItem(int line, const TemplateInfo& templateInfo, bool selected);
    void RenderActionItem(int line, const std::string& text, bool selected);
    bool NeedsRedraw() const;


    void FindNextAvailableSlot();
    void FindPreviousAvailableSlot();

    void LoadTemplates();
    void CreateNewTemplate();
    void EditTemplate(int slot);
    void DeleteTemplate(int slot);

    std::vector<TemplateInfo> m_templates;
    std::vector<std::string> m_emptyTemplateActions;
    std::vector<std::string> m_usedTemplateActions;
    int m_selectedSlot;
    int m_selectedActionIndex;
    bool m_shouldReturn;
    bool m_shouldCreateTemplate;
    std::unique_ptr<TemplateSystem> m_templateSystem;
    std::unique_ptr<WorldEditor> m_worldEditor;

    int m_prevSelectedSlot;
    int m_prevSelectedActionIndex;
    TemplateActionState m_currentState;
    TemplateActionState m_prevState;
    std::vector<TemplateInfo> m_prevTemplates;
    bool m_needFullRedraw;
    bool m_ignoreFirstInput;
    std::unique_ptr<InputManager> m_inputManager;

    bool m_showActionsForSlot;
    int m_actionSlot;

    void RenderTemplatesGrid();

    int m_currentColumn;
    int m_currentRow;
};