#include "WorldTemplateMenu.h"
#include <windows.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>
#include "Logger.h"
#include "HelpPanel.h"
#include "HelpSystem.h"

namespace rlutil {
    void setColor(int color);
    void cls();
    void locate(int x, int y);
    void hideCursor();
}

WorldTemplateMenu::WorldTemplateMenu()
    : m_selectedSlot(1)
    , m_selectedActionIndex(0)
    , m_shouldReturn(false)
    , m_shouldCreateTemplate(false)
    , m_prevSelectedSlot(-1)
    , m_prevSelectedActionIndex(-1)
    , m_currentState(TemplateActionState::MAIN_LIST)
    , m_prevState(TemplateActionState::MAIN_LIST)
    , m_needFullRedraw(true)
    , m_ignoreFirstInput(true)
    , m_showActionsForSlot(false)
    , m_actionSlot(-1)
    , m_currentColumn(0)
    , m_currentRow(0)
{
    Logger::Log("=== WorldTemplateMenu CONSTRUCTOR ===");

    m_templateSystem = std::make_unique<TemplateSystem>();
    m_templateSystem->Initialize();

    m_inputManager = std::make_unique<InputManager>();

    m_emptyTemplateActions = {
        "Create",
        "Back"
    };

    m_usedTemplateActions = {
        "Edit",
        "Delete",
        "Load",
        "Back"
    };

    HelpPanel::Initialize();
    auto& helpSystem = HelpSystem::GetInstance();
    helpSystem.RegisterTemplateMenuHelp();

    Logger::Log("WorldTemplateMenu constructor completed");
    Logger::Log("=== END CONSTRUCTOR ===");
}

void WorldTemplateMenu::Initialize() {
    rlutil::hideCursor();

    m_currentState = TemplateActionState::MAIN_LIST;
    m_prevState = TemplateActionState::MAIN_LIST;
    m_selectedSlot = 1;
    m_selectedActionIndex = 0;
    m_needFullRedraw = true;
    m_ignoreFirstInput = true;
    m_showActionsForSlot = false;
    m_actionSlot = -1;
    m_currentColumn = 0;
    m_currentRow = 0;

    LoadTemplates();

    if (m_inputManager) {
        m_inputManager->ClearSystemBuffer();
        m_inputManager->ClearState();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    Logger::Log("WorldTemplateMenu Initialize() - State reset to MAIN_LIST");
}

void WorldTemplateMenu::LoadTemplates() {
    Logger::Log("=== LOADING TEMPLATES ===");

    std::string templatesDir = "templates/";
    Logger::Log("Checking templates directory: " + templatesDir);

    if (!fs::exists(templatesDir)) {
        Logger::Log("WARNING: Templates directory doesn't exist!");
        Logger::Log("Creating templates directory...");
        fs::create_directories(templatesDir);
    }

    for (int i = 1; i <= 30; i++) {
        std::string templateDir = templatesDir + "template" + std::to_string(i) + "/";
        if (fs::exists(templateDir)) {
            Logger::Log("Template directory " + std::to_string(i) + " exists");

            int fileCount = 0;
            for (const auto& entry : fs::directory_iterator(templateDir)) {
                if (entry.is_regular_file()) {
                    Logger::Log("  File: " + entry.path().filename().string());
                    fileCount++;
                }
            }
            Logger::Log("  Total files: " + std::to_string(fileCount));
        }
    }

    auto systemTemplates = m_templateSystem->GetTemplates();

    m_templates.clear();

    const int TOTAL_TEMPLATES = 30;

    for (int i = 1; i <= TOTAL_TEMPLATES; i++) {
        TemplateInfo info = m_templateSystem->GetTemplateInfo(i);

        Logger::Log("Slot " + std::to_string(i) + ": " +
            (info.isEmpty ? "Empty" : info.name) +
            ", Path: " + info.templatePath);

        if (!info.isEmpty) {
            std::string templateDir = info.templatePath;
            if (fs::exists(templateDir)) {
                int fileCount = 0;
                for (const auto& entry : fs::directory_iterator(templateDir)) {
                    if (entry.is_regular_file()) {
                        fileCount++;
                    }
                }
                Logger::Log("  Files in template: " + std::to_string(fileCount));
            }
        }

        m_templates.push_back(info);
    }

    std::sort(m_templates.begin(), m_templates.end(), [](const TemplateInfo& a, const TemplateInfo& b) {
        return a.slotNumber < b.slotNumber;
        });

    Logger::Log("Loaded " + std::to_string(m_templates.size()) + " templates");
    Logger::Log("=== END LOADING TEMPLATES ===");
}

void WorldTemplateMenu::Update() {
    if (m_inputManager) {
        m_inputManager->Update();
    }

    if (m_currentState == TemplateActionState::WORLD_EDITOR && m_worldEditor) {
        m_worldEditor->Update();
    }
}

void WorldTemplateMenu::Render() {
    if (m_currentState == TemplateActionState::WORLD_EDITOR && m_worldEditor) {
        m_worldEditor->Render();
        return;
    }

    if (m_currentState != m_prevState) {
        Logger::Log("WorldTemplateMenu State changed from " + std::to_string(static_cast<int>(m_prevState)) +
            " to " + std::to_string(static_cast<int>(m_currentState)));
        rlutil::cls();
        m_needFullRedraw = true;
        m_prevState = m_currentState;
    }

    if (m_needFullRedraw) {
        rlutil::cls();
    }

    UpdateHelpForCurrentSelection();
    RenderOnlyChanges();

    m_prevSelectedSlot = m_selectedSlot;
    m_prevSelectedActionIndex = m_selectedActionIndex;
    m_prevTemplates = m_templates;

    static bool prevShowActions = false;
    if (m_showActionsForSlot != prevShowActions) {
        m_needFullRedraw = true;
        prevShowActions = m_showActionsForSlot;
    }

    int screenHeight = 25;
    int screenWidth = 80;
    HelpPanel::Render(screenWidth, screenHeight);

    m_needFullRedraw = false;
}

bool WorldTemplateMenu::NeedsRedraw() const {
    static bool prevShowActionsState = false;
    bool showActionsChanged = (m_showActionsForSlot != prevShowActionsState);

    return m_prevSelectedSlot != m_selectedSlot ||
        m_prevSelectedActionIndex != m_selectedActionIndex ||
        m_currentState != m_prevState ||
        m_showActionsForSlot != (m_actionSlot != -1) ||
        showActionsChanged;
}

void WorldTemplateMenu::RenderOnlyChanges() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    if (m_needFullRedraw) {
        SetConsoleTextAttribute(hConsole, 14);
        rlutil::locate(2, 1);
        std::cout << "World Templates";

        SetConsoleTextAttribute(hConsole, 7);
        rlutil::locate(2, 3);
        std::cout << "Templates";
        rlutil::locate(2, 4);
        std::cout << "------------------------------------------------------------";
    }

    RenderTemplatesGrid();

    if (m_needFullRedraw) {
        rlutil::locate(2, 22);
        if (m_showActionsForSlot) {
            std::cout << "Controls: W/S - Select action, SPACE/ENTER - Confirm, Q/ESC - Back";
        }
        else {
            std::cout << "Controls: W/S/A/D - Navigate, SPACE/ENTER - Select, Q/ESC - Back";
        }
    }

    SetConsoleTextAttribute(hConsole, 7);
}

void WorldTemplateMenu::RenderTemplatesGrid() {
    const int COLUMNS = 5;
    const int ROWS = 6;
    const int TOTAL_TEMPLATES = 30;
    int startLine = 6;

    if (m_showActionsForSlot) {
        TemplateInfo selectedTemplate;
        for (const auto& tmpl : m_templates) {
            if (tmpl.slotNumber == m_actionSlot) {
                selectedTemplate = tmpl;
                break;
            }
        }

        const auto& actions = selectedTemplate.isEmpty ? m_emptyTemplateActions : m_usedTemplateActions;

        if (m_needFullRedraw) {
            for (int i = startLine; i <= startLine + ROWS + 2; i++) {
                ClearLine(i);
            }

            rlutil::locate(2, startLine);
            std::cout << "Template " << m_actionSlot << " actions:";
        }

        for (size_t i = 0; i < actions.size(); ++i) {
            int line = startLine + 1 + static_cast<int>(i);
            bool actionSelected = (static_cast<int>(i) == m_selectedActionIndex);

            if (m_needFullRedraw || (static_cast<int>(i) == m_selectedActionIndex) ||
                (static_cast<int>(i) == m_prevSelectedActionIndex)) {
                RenderActionItem(line, actions[i], actionSelected);
            }
        }
    }
    else {
        if (m_needFullRedraw) {
            for (int i = startLine; i <= startLine + ROWS + 2; i++) {
                ClearLine(i);
            }
        }

        for (int row = 0; row < ROWS; row++) {
            for (int col = 0; col < COLUMNS; col++) {
                int slotNumber = row + 1 + col * ROWS;

                if (slotNumber > TOTAL_TEMPLATES) {
                    continue;
                }

                TemplateInfo templateInfo;
                bool found = false;
                for (const auto& tmpl : m_templates) {
                    if (tmpl.slotNumber == slotNumber) {
                        templateInfo = tmpl;
                        found = true;
                        break;
                    }
                }

                if (!found) {
                    templateInfo.slotNumber = slotNumber;
                    templateInfo.name = "Empty";
                    templateInfo.isEmpty = true;
                }

                int xPos = 4 + (col * 16);
                int yPos = startLine + row;

                bool isSelected = (m_selectedSlot == slotNumber);
                bool wasSelected = (m_prevSelectedSlot == slotNumber);

                if (m_needFullRedraw || isSelected != wasSelected) {
                    rlutil::locate(xPos, yPos);
                    for (int j = 0; j < 15; j++) {
                        std::cout << ' ';
                    }
                    rlutil::locate(xPos, yPos);

                    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

                    if (isSelected) {
                        SetConsoleTextAttribute(hConsole, 10);
                        std::cout << ">";
                    }
                    else {
                        SetConsoleTextAttribute(hConsole, 7);
                        std::cout << " ";
                    }

                    std::string displayName = std::to_string(templateInfo.slotNumber) + ". ";
                    if (templateInfo.name.length() > 8) {
                        displayName += templateInfo.name.substr(0, 8) + "..";
                    }
                    else {
                        displayName += templateInfo.name;
                    }

                    std::cout << " " << displayName;

                    SetConsoleTextAttribute(hConsole, 7);
                }
            }
        }

        int backLine = startLine + ROWS + 1;
        bool backSelected = (m_selectedSlot == TOTAL_TEMPLATES + 1);
        bool backWasSelected = (m_prevSelectedSlot == TOTAL_TEMPLATES + 1);

        if (m_needFullRedraw || backSelected != backWasSelected) {
            RenderActionItem(backLine, "Back", backSelected);
        }
    }
}


int GetGridIndex(int slot, int column, int totalRows) {
    int row = (slot - 1) % totalRows;
    int col = (slot - 1) / totalRows;
    return row * 5 + col;
}


void WorldTemplateMenu::RenderTemplateItem(int line, const TemplateInfo& templateInfo, bool selected) {
    rlutil::locate(4, line);

    std::cout << "                                                                                ";
    rlutil::locate(4, line);

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    if (selected) {
        SetConsoleTextAttribute(hConsole, 10);
        std::cout << "> ";
    }
    else {
        SetConsoleTextAttribute(hConsole, 7);
        std::cout << "  ";
    }

    std::cout << templateInfo.slotNumber << ". " << templateInfo.GetDisplayName();

    SetConsoleTextAttribute(hConsole, 7);
}

void WorldTemplateMenu::RenderActionItem(int line, const std::string& text, bool selected) {
    rlutil::locate(4, line);

    std::cout << "                                                                                ";
    rlutil::locate(4, line);

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    if (selected) {
        SetConsoleTextAttribute(hConsole, 10);
        std::cout << "> " << text;
    }
    else {
        SetConsoleTextAttribute(hConsole, 7);
        std::cout << "  " << text;
    }

    SetConsoleTextAttribute(hConsole, 7);
}

void WorldTemplateMenu::ProcessInput() {
    if (!m_inputManager) return;

    if (m_currentState == TemplateActionState::WORLD_EDITOR && m_worldEditor) {
        m_worldEditor->ProcessInput();

        if (m_worldEditor->ShouldReturnToSaves()) {
            m_currentState = TemplateActionState::MAIN_LIST;
            m_worldEditor.reset();
            m_needFullRedraw = true;
            m_showActionsForSlot = false;
            m_actionSlot = -1;
            LoadTemplates();
            Logger::Log("World Editor closed, returning to template selection");
        }
        else if (m_worldEditor->ShouldCreateTemplate()) {
            m_currentState = TemplateActionState::MAIN_LIST;
            m_worldEditor.reset();
            m_needFullRedraw = true;
            m_showActionsForSlot = false;
            m_actionSlot = -1;
            LoadTemplates();
            Logger::Log("Template created, returning to template selection");
        }
        return;
    }

    if (m_showActionsForSlot) {
        if (m_inputManager->IsMenuUp() || m_inputManager->IsKeyPressed('W')) {
            TemplateInfo selectedTemplate;
            for (const auto& templateInfo : m_templates) {
                if (templateInfo.slotNumber == m_actionSlot) {
                    selectedTemplate = templateInfo;
                    break;
                }
            }
            const auto& actions = selectedTemplate.isEmpty ? m_emptyTemplateActions : m_usedTemplateActions;
            m_selectedActionIndex = (m_selectedActionIndex - 1 + actions.size()) % actions.size();
        }
        else if (m_inputManager->IsMenuDown() || m_inputManager->IsKeyPressed('S')) {
            TemplateInfo selectedTemplate;
            for (const auto& templateInfo : m_templates) {
                if (templateInfo.slotNumber == m_actionSlot) {
                    selectedTemplate = templateInfo;
                    break;
                }
            }
            const auto& actions = selectedTemplate.isEmpty ? m_emptyTemplateActions : m_usedTemplateActions;
            m_selectedActionIndex = (m_selectedActionIndex + 1) % actions.size();
        }
        else if (m_inputManager->IsMenuSelect()) {
            ConfirmSelection();
        }
        else if (m_inputManager->IsMenuBack()) {
            m_showActionsForSlot = false;
            m_actionSlot = -1;
            m_selectedActionIndex = 0;
            m_needFullRedraw = true;
            Logger::Log("WorldTemplateMenu Back from actions to MAIN_LIST");
        }
    }
    else {
        if (m_inputManager->IsMenuUp() || m_inputManager->IsKeyPressed('W')) {
            SelectPreviousOption();
        }
        else if (m_inputManager->IsMenuDown() || m_inputManager->IsKeyPressed('S')) {
            SelectNextOption();
        }
        else if (m_inputManager->IsKeyPressed('A') || m_inputManager->IsKeyPressed(VK_LEFT)) {
            SelectLeftOption();
        }
        else if (m_inputManager->IsKeyPressed('D') || m_inputManager->IsKeyPressed(VK_RIGHT)) {
            SelectRightOption();
        }
        else if (m_inputManager->IsMenuSelect()) {
            ConfirmSelection();
        }
        else if (m_inputManager->IsMenuBack()) {
            m_shouldReturn = true;
            Logger::Log("WorldTemplateMenu Returning to main menu");
        }
    }
}

void WorldTemplateMenu::SelectNextOption() {
    if (m_showActionsForSlot) {
        TemplateInfo selectedTemplate;
        for (const auto& templateInfo : m_templates) {
            if (templateInfo.slotNumber == m_actionSlot) {
                selectedTemplate = templateInfo;
                break;
            }
        }
        const auto& actions = selectedTemplate.isEmpty ? m_emptyTemplateActions : m_usedTemplateActions;
        m_selectedActionIndex = (m_selectedActionIndex + 1) % actions.size();
    }
    else {
        const int ROWS = 6;
        const int COLUMNS = 5;
        const int TOTAL_TEMPLATES = 30;

        if (m_selectedSlot == TOTAL_TEMPLATES + 1) {
            m_currentRow = ROWS - 1;
            m_currentColumn = 0;
            m_selectedSlot = m_currentRow + 1 + m_currentColumn * ROWS;
            Logger::Log("WorldTemplateMenu: Back -> Slot " + std::to_string(m_selectedSlot));
            return;
        }

        if (m_selectedSlot <= TOTAL_TEMPLATES) {
            m_currentRow = (m_selectedSlot - 1) % ROWS;
            m_currentColumn = (m_selectedSlot - 1) / ROWS;

            if (m_currentRow < ROWS - 1) {
                m_currentRow++;
                int newSlot = m_currentRow + 1 + m_currentColumn * ROWS;

                if (newSlot <= TOTAL_TEMPLATES) {
                    m_selectedSlot = newSlot;
                    Logger::Log("WorldTemplateMenu: Down -> Slot " + std::to_string(m_selectedSlot));
                }
                else {
                    FindNextAvailableSlot();
                }
            }
            else {
                m_selectedSlot = TOTAL_TEMPLATES + 1;
                Logger::Log("WorldTemplateMenu: Last row -> Back");
            }
        }
    }
}

void WorldTemplateMenu::SelectPreviousOption() {
    if (m_showActionsForSlot) {
        TemplateInfo selectedTemplate;
        for (const auto& templateInfo : m_templates) {
            if (templateInfo.slotNumber == m_actionSlot) {
                selectedTemplate = templateInfo;
                break;
            }
        }
        const auto& actions = selectedTemplate.isEmpty ? m_emptyTemplateActions : m_usedTemplateActions;
        m_selectedActionIndex = (m_selectedActionIndex - 1 + actions.size()) % actions.size();
    }
    else {
        const int ROWS = 6;
        const int TOTAL_TEMPLATES = 30;

        if (m_selectedSlot == TOTAL_TEMPLATES + 1) {
            m_currentRow = ROWS - 1;
            m_currentColumn = 0;
            m_selectedSlot = m_currentRow + 1 + m_currentColumn * ROWS;
            Logger::Log("WorldTemplateMenu: Back -> Slot " + std::to_string(m_selectedSlot) + " (6. Empty)");
            return;
        }

        if (m_selectedSlot <= TOTAL_TEMPLATES) {
            m_currentRow = (m_selectedSlot - 1) % ROWS;
            m_currentColumn = (m_selectedSlot - 1) / ROWS;

            if (m_currentRow > 0) {
                m_currentRow--;
                m_selectedSlot = m_currentRow + 1 + m_currentColumn * ROWS;
                Logger::Log("WorldTemplateMenu: Up -> Slot " + std::to_string(m_selectedSlot));
            }
            else {
                if (m_currentColumn > 0) {
                    m_currentColumn--;
                    m_currentRow = ROWS - 1;
                    int newSlot = m_currentRow + 1 + m_currentColumn * ROWS;

                    if (newSlot <= TOTAL_TEMPLATES) {
                        m_selectedSlot = newSlot;
                        Logger::Log("WorldTemplateMenu: Up to previous column -> Slot " + std::to_string(m_selectedSlot));
                    }
                    else {
                        FindPreviousAvailableSlot();
                    }
                }
                else {
                    m_selectedSlot = TOTAL_TEMPLATES + 1;
                    Logger::Log("WorldTemplateMenu: First slot -> Back");
                }
            }
        }
    }
}

void WorldTemplateMenu::FindNextAvailableSlot() {
    const int ROWS = 6;
    const int COLUMNS = 5;
    const int TOTAL_TEMPLATES = 30;

    if (m_currentColumn < COLUMNS - 1) {
        m_currentColumn++;
        m_currentRow = 0;
        int newSlot = m_currentRow + 1 + m_currentColumn * ROWS;

        if (newSlot <= TOTAL_TEMPLATES) {
            m_selectedSlot = newSlot;
            Logger::Log("WorldTemplateMenu: Next column -> Slot " + std::to_string(m_selectedSlot));
            return;
        }
    }

    m_selectedSlot = TOTAL_TEMPLATES + 1;
    Logger::Log("WorldTemplateMenu: No more slots -> Back");
}

void WorldTemplateMenu::FindPreviousAvailableSlot() {
    const int ROWS = 6;
    const int TOTAL_TEMPLATES = 30;

    while (m_currentRow >= 0) {
        int testSlot = m_currentRow + 1 + m_currentColumn * ROWS;
        if (testSlot <= TOTAL_TEMPLATES) {
            m_selectedSlot = testSlot;
            Logger::Log("WorldTemplateMenu: Found previous slot -> Slot " + std::to_string(m_selectedSlot));
            return;
        }
        m_currentRow--;
    }

    Logger::Log("WorldTemplateMenu: Could not find previous slot");
}

void WorldTemplateMenu::SelectLeftOption() {
    if (m_showActionsForSlot || m_selectedSlot > 30) return;

    const int ROWS = 6;
    const int TOTAL_TEMPLATES = 30;

    m_currentRow = (m_selectedSlot - 1) % ROWS;
    m_currentColumn = (m_selectedSlot - 1) / ROWS;

    if (m_currentColumn > 0) {
        m_currentColumn--;
        int newSlot = m_currentRow + 1 + m_currentColumn * ROWS;

        if (newSlot <= TOTAL_TEMPLATES) {
            m_selectedSlot = newSlot;
        }
        else {
            int lastRowInColumn = (TOTAL_TEMPLATES - 1 - m_currentColumn * ROWS) % ROWS;
            if (m_currentRow > lastRowInColumn) {
                m_currentRow = lastRowInColumn;
            }
            m_selectedSlot = m_currentRow + 1 + m_currentColumn * ROWS;
        }
    }
    else {
        m_currentColumn = 4;
        int newSlot = m_currentRow + 1 + m_currentColumn * ROWS;

        if (newSlot <= TOTAL_TEMPLATES) {
            m_selectedSlot = newSlot;
        }
        else {
            m_currentColumn = 4;
            while (m_currentColumn > 0) {
                int testSlot = m_currentRow + 1 + m_currentColumn * ROWS;
                if (testSlot <= TOTAL_TEMPLATES) {
                    m_selectedSlot = testSlot;
                    break;
                }
                m_currentColumn--;
            }
        }
    }

    Logger::Log("WorldTemplateMenu SelectLeft - Slot: " + std::to_string(m_selectedSlot) +
        ", Row: " + std::to_string(m_currentRow) + ", Col: " + std::to_string(m_currentColumn));
}

void WorldTemplateMenu::SelectRightOption() {
    if (m_showActionsForSlot || m_selectedSlot > 30) return;

    const int ROWS = 6;
    const int TOTAL_TEMPLATES = 30;

    m_currentRow = (m_selectedSlot - 1) % ROWS;
    m_currentColumn = (m_selectedSlot - 1) / ROWS;

    if (m_currentColumn < 4) {
        m_currentColumn++;
        int newSlot = m_currentRow + 1 + m_currentColumn * ROWS;

        if (newSlot <= TOTAL_TEMPLATES) {
            m_selectedSlot = newSlot;
        }
        else {
            m_currentColumn--;
        }
    }
    else {
        m_currentColumn = 0;
        m_selectedSlot = m_currentRow + 1 + m_currentColumn * ROWS;
    }

    Logger::Log("WorldTemplateMenu SelectRight - Slot: " + std::to_string(m_selectedSlot) +
        ", Row: " + std::to_string(m_currentRow) + ", Col: " + std::to_string(m_currentColumn));
}

void WorldTemplateMenu::ConfirmSelection() {
    Logger::Log("WorldTemplateMenu ConfirmSelection - State: " +
        std::to_string(static_cast<int>(m_currentState)) +
        ", Slot: " + std::to_string(m_selectedSlot) +
        ", ShowActions: " + std::to_string(m_showActionsForSlot));

    if (m_showActionsForSlot) {
        TemplateInfo selectedTemplate;
        for (const auto& templateInfo : m_templates) {
            if (templateInfo.slotNumber == m_actionSlot) {
                selectedTemplate = templateInfo;
                break;
            }
        }

        const auto& actions = selectedTemplate.isEmpty ? m_emptyTemplateActions : m_usedTemplateActions;

        if (m_selectedActionIndex >= actions.size()) {
            Logger::Log("WorldTemplateMenu ERROR: Action index out of bounds");
            return;
        }

        std::string selectedAction = actions[m_selectedActionIndex];
        Logger::Log("WorldTemplateMenu Action selected: " + selectedAction + " for slot " +
            std::to_string(m_actionSlot));

        if (selectedAction == "Back") {
            m_showActionsForSlot = false;
            m_actionSlot = -1;
            m_selectedActionIndex = 0;
            m_needFullRedraw = true;
        }
        else if (selectedAction == "Create") {
            CreateNewTemplate();
        }
        else if (selectedAction == "Edit") {
            EditTemplate(m_actionSlot);
        }
        else if (selectedAction == "Delete") {
            DeleteTemplate(m_actionSlot);
        }
        else if (selectedAction == "Load") {
            Logger::Log("Preview template " + std::to_string(m_actionSlot));
        }
    }
    else {
        const int TOTAL_TEMPLATES = 30;

        if (m_selectedSlot == TOTAL_TEMPLATES + 1) {
            Logger::Log("WorldTemplateMenu Back selected - returning to main menu");
            m_shouldReturn = true;
            return;
        }

        TemplateInfo selectedTemplate;
        for (const auto& templateInfo : m_templates) {
            if (templateInfo.slotNumber == m_selectedSlot) {
                selectedTemplate = templateInfo;
                break;
            }
        }

        if (selectedTemplate.isEmpty) {
            m_showActionsForSlot = true;
            m_actionSlot = m_selectedSlot;
            m_selectedActionIndex = 0;
            m_needFullRedraw = true;
            Logger::Log("Showing actions for empty template slot " + std::to_string(m_actionSlot));
        }
        else {
            m_showActionsForSlot = true;
            m_actionSlot = m_selectedSlot;
            m_selectedActionIndex = 0;
            m_needFullRedraw = true;
            Logger::Log("Showing actions for template slot " + std::to_string(m_actionSlot));
        }
    }
}

void WorldTemplateMenu::CreateNewTemplate() {
    Logger::Log("Creating new template in slot " + std::to_string(m_actionSlot));

    int slotToUse = m_actionSlot;

    m_worldEditor = std::make_unique<WorldEditor>(
        EditorMode::CREATE_TEMPLATE,
        slotToUse
    );

    std::string tempTilesPath = "templates\template_tiles_" + std::to_string(slotToUse) + ".json";
    if (fs::exists(tempTilesPath)) {
        Logger::Log("Using existing tile config from: " + tempTilesPath);
    }
    else {
        Logger::Log("Creating default tile config at: " + tempTilesPath);
    }

    m_worldEditor->Initialize();
    m_currentState = TemplateActionState::WORLD_EDITOR;
    rlutil::cls();
    m_needFullRedraw = true;
    m_showActionsForSlot = false;
    m_actionSlot = -1;

    Logger::Log("Opening World Editor for template creation in slot " + std::to_string(slotToUse));
}

void WorldTemplateMenu::EditTemplate(int slot) {
    Logger::Log("Editing template in slot " + std::to_string(slot));

    m_worldEditor = std::make_unique<WorldEditor>(
        EditorMode::CREATE_TEMPLATE,
        slot
    );

    WorldEditorConfig templateConfig;
    if (m_templateSystem->LoadTemplate(slot, templateConfig)) {
        Logger::Log("Loaded template configuration");
    }

    m_worldEditor->Initialize();
    m_currentState = TemplateActionState::WORLD_EDITOR;
    rlutil::cls();
    m_needFullRedraw = true;
    m_showActionsForSlot = false;
    m_actionSlot = -1;

    Logger::Log("Opening World Editor for template editing in slot " + std::to_string(slot));
}

void WorldTemplateMenu::DeleteTemplate(int slot) {
    Logger::Log("Deleting template in slot " + std::to_string(slot));

    if (m_templateSystem->DeleteTemplate(slot)) {
        Logger::Log("Successfully deleted template slot " + std::to_string(slot));
        LoadTemplates();

        m_showActionsForSlot = false;
        m_actionSlot = -1;
        m_needFullRedraw = true;

        Logger::Log("Template deleted, returning to templates list");
    }
    else {
        Logger::Log("ERROR: Failed to delete template slot " + std::to_string(slot));
    }
}

void WorldTemplateMenu::ClearLine(int line) {
    rlutil::locate(0, line);
    for (int i = 0; i < 80; i++) {
        std::cout << ' ';
    }
}

void WorldTemplateMenu::Reset() {
    Logger::Log("WorldTemplateMenu Reset() called");

    m_selectedSlot = 1;
    m_selectedActionIndex = 0;
    m_shouldReturn = false;
    m_shouldCreateTemplate = false;
    m_currentState = TemplateActionState::MAIN_LIST;
    m_needFullRedraw = true;
    m_showActionsForSlot = false;
    m_actionSlot = -1;
    m_currentColumn = 0;
    m_currentRow = 0;

    LoadTemplates();

    Logger::Log("WorldTemplateMenu Reset completed");
}

void WorldTemplateMenu::UpdateHelpForCurrentSelection() {
    auto& helpSystem = HelpSystem::GetInstance();
    std::string currentItemId;

    if (m_showActionsForSlot) {
        TemplateInfo selectedTemplate;
        for (const auto& tmpl : m_templates) {
            if (tmpl.slotNumber == m_actionSlot) {
                selectedTemplate = tmpl;
                break;
            }
        }

        if (selectedTemplate.isEmpty) {
            switch (m_selectedActionIndex) {
            case 0: currentItemId = "Create"; break;
            case 1: currentItemId = "Back"; break;
            }
        }
        else {
            switch (m_selectedActionIndex) {
            case 0: currentItemId = "Edit"; break;
            case 1: currentItemId = "Delete"; break;
            case 2: currentItemId = "Load"; break;
            case 3: currentItemId = "Back"; break;
            }
        }
    }
    else {
        const int TOTAL_TEMPLATES = 30;
        if (m_selectedSlot <= TOTAL_TEMPLATES) {
            currentItemId = "template_slot_" + std::to_string(m_selectedSlot);
        }
        else {
            currentItemId = "Back";
        }
    }

    std::string helpText = helpSystem.GetHelpForItem(currentItemId);
    if (!helpText.empty()) {
        HelpPanel::SetHelpText(helpText);
    }
    else {
        HelpPanel::ClearHelpText();
    }
}