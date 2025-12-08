#include "SaveSelectionMenu.h"
#include <windows.h>
#include <iostream>
#include <chrono>
#include <thread>
#include "Logger.h"
#include "HelpPanel.h"
#include "HelpSystem.h"

namespace rlutil {
    void setColor(int color);
    void cls();
    void locate(int x, int y);
    void hideCursor();
}

SaveSelectionMenu::SaveSelectionMenu()
    : m_gameMode(GameMode::PROCEDURAL_GENERATION)
    , m_selectedSlot(1)
    , m_selectedActionIndex(0)
    , m_shouldReturn(false)
    , m_shouldStartGame(false)
    , m_prevSelectedSlot(-1)
    , m_prevSelectedActionIndex(-1)
    , m_currentState(SaveActionState::MAIN_LIST)
    , m_prevState(SaveActionState::MAIN_LIST)
    , m_needFullRedraw(true)
    , m_ignoreFirstInput(true)
    , m_showActionsForSlot(false)
    , m_actionSlot(-1)
    , m_selectedTemplateIndex(0)
    , m_templateForSlot(-1)
    , m_inTemplateSelection(false)
    , m_prevSelectedTemplateIndex(-1)
    , m_prevBackSelected(false)
{
    Logger::Log("=== SaveSelectionMenu CONSTRUCTOR ===");

    m_saveSystem = std::make_unique<SaveSystem>();
    m_templateSystem = std::make_unique<TemplateSystem>();
    m_templateSystem->Initialize();

    std::vector<SaveInfo> existingSaves = m_saveSystem->GetAllSaves();

    const int TOTAL_SLOTS = 5;
    m_saves.clear();

    for (int slot = 1; slot <= TOTAL_SLOTS; slot++) {
        bool slotExists = false;
        SaveInfo slotSave;

        for (const auto& save : existingSaves) {
            if (save.slotNumber == slot) {
                slotExists = true;
                slotSave = save;
                break;
            }
        }

        if (slotExists) {
            m_saves.push_back(slotSave);
        }
        else {
            SaveInfo emptySave;
            emptySave.slotNumber = slot;
            emptySave.name = "";
            emptySave.gameMode = GameMode::PROCEDURAL_GENERATION;
            emptySave.isEmpty = true;
            emptySave.creationDate = "";
            emptySave.lastPlayedDate = "";
            emptySave.savePath = "";
            emptySave.templateId = -1;

            m_saves.push_back(emptySave);
        }
    }

    std::sort(m_saves.begin(), m_saves.end(), [](const SaveInfo& a, const SaveInfo& b) {
        return a.slotNumber < b.slotNumber;
        });

    m_prevSaves = m_saves;
    m_inputManager = std::make_unique<InputManager>();

    m_emptySaveActions = {
        "Create",
        "Cancel"
    };

    m_usedSaveActions = {
        "Play",
        "Delete",
        "Cancel"
    };

    Logger::Log("Constructor completed. Total slots: " + std::to_string(m_saves.size()));
    for (const auto& save : m_saves) {
        Logger::Log("Slot " + std::to_string(save.slotNumber) +
            ": " + (save.isEmpty ? "Empty" : save.name));
    }

    HelpPanel::Initialize();
    auto& helpSystem = HelpSystem::GetInstance();
    helpSystem.RegisterSaveMenuHelp();

    Logger::Log("=== END CONSTRUCTOR ===");
}

void SaveSelectionMenu::Initialize() {
    rlutil::hideCursor();

    m_currentState = SaveActionState::MAIN_LIST;
    m_prevState = SaveActionState::MAIN_LIST;
    m_selectedSlot = 1;
    m_selectedActionIndex = 0;
    m_needFullRedraw = true;
    m_ignoreFirstInput = true;
    m_showActionsForSlot = false;
    m_actionSlot = -1;
    m_selectedTemplateIndex = 0;
    m_templateForSlot = -1;
    m_inTemplateSelection = false;

    if (m_inputManager) {
        m_inputManager->ClearSystemBuffer();
        m_inputManager->ClearState();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    Logger::Log("SaveSelectionMenu Initialize() - State reset to MAIN_LIST");
}

void SaveSelectionMenu::Update() {
    if (m_inputManager) {
        m_inputManager->Update();
    }

    if (m_currentState == SaveActionState::WORLD_EDITOR && m_worldEditor) {
        m_worldEditor->Update();
    }
}

void SaveSelectionMenu::Render() {
    if (m_currentState == SaveActionState::WORLD_EDITOR && m_worldEditor) {
        m_worldEditor->Render();
        return;
    }

    if (m_currentState != m_prevState) {
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
    m_prevSaves = m_saves;

    if (m_inTemplateSelection) {
        m_prevSelectedTemplateIndex = m_selectedTemplateIndex;
        m_prevBackSelected = (m_selectedTemplateIndex == 30);
    }

    int screenHeight = 25;
    int screenWidth = 80;
    HelpPanel::Render(screenWidth, screenHeight);

    m_needFullRedraw = false;
}

bool SaveSelectionMenu::NeedsRedraw() const {
    if (m_inTemplateSelection) {
        return m_prevSelectedSlot != m_selectedSlot ||
            m_prevSelectedActionIndex != m_selectedActionIndex ||
            m_currentState != m_prevState ||
            m_showActionsForSlot != (m_actionSlot != -1) ||
            m_inTemplateSelection != (m_templateForSlot != -1) ||
            m_prevSelectedTemplateIndex != m_selectedTemplateIndex;
    }
    else {
        return m_prevSelectedSlot != m_selectedSlot ||
            m_prevSelectedActionIndex != m_selectedActionIndex ||
            m_currentState != m_prevState ||
            m_showActionsForSlot != (m_actionSlot != -1) ||
            m_inTemplateSelection != (m_templateForSlot != -1);
    }
}

void SaveSelectionMenu::RenderOnlyChanges() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    if (m_needFullRedraw) {
        SetConsoleTextAttribute(hConsole, 14);
        rlutil::locate(2, 1);
        if (m_inTemplateSelection) {
            std::cout << "Select Template for Slot " << m_templateForSlot;
        }
        else {
            std::cout << "Game Saves";
        }

        SetConsoleTextAttribute(hConsole, 7);
        rlutil::locate(2, 3);
        if (m_inTemplateSelection) {
            std::cout << "Templates";
        }
        else {
            std::cout << "Saves";
        }
        rlutil::locate(2, 4);
        std::cout << "------------------------------------------------------------";
    }

    if (m_inTemplateSelection) {
        RenderTemplatesList();
    }
    else {
        RenderSavesList();
    }

    if (m_needFullRedraw) {
        rlutil::locate(2, 22);
        if (m_inTemplateSelection) {
            std::cout << "Controls: W/S/A/D - Navigate, SPACE/ENTER - Select, Q/ESC - Back";
        }
        else if (m_showActionsForSlot) {
            std::cout << "Controls: W/S - Select action, SPACE/ENTER - Confirm, Q/ESC - Back";
        }
        else {
            std::cout << "Controls: W/S - Navigate, SPACE/ENTER - Select, Q/ESC - Back";
        }
    }

    SetConsoleTextAttribute(hConsole, 7);
}

void SaveSelectionMenu::RenderSavesList() {
    int currentLine = 6;

    if (m_needFullRedraw) {
        for (int i = 6; i <= 20; i++) {
            ClearLine(i);
        }
    }

    const int TOTAL_SLOTS = 5;

    for (int slot = 1; slot <= TOTAL_SLOTS; slot++) {
        SaveInfo saveForSlot;
        bool slotExists = false;

        for (const auto& save : m_saves) {
            if (save.slotNumber == slot) {
                saveForSlot = save;
                slotExists = true;
                break;
            }
        }

        if (!slotExists) {
            saveForSlot.slotNumber = slot;
            saveForSlot.name = "";
            saveForSlot.gameMode = GameMode::PROCEDURAL_GENERATION;
            saveForSlot.isEmpty = true;
            saveForSlot.creationDate = "";
            saveForSlot.lastPlayedDate = "";
            saveForSlot.savePath = "";
            saveForSlot.templateId = -1;
        }

        bool isSelected = (saveForSlot.slotNumber == m_selectedSlot && !m_showActionsForSlot);

        RenderSaveItem(currentLine, saveForSlot, isSelected);

        if (m_showActionsForSlot && saveForSlot.slotNumber == m_actionSlot) {
            const auto& actions = saveForSlot.isEmpty ? m_emptySaveActions : m_usedSaveActions;

            for (size_t actionIndex = 0; actionIndex < actions.size(); ++actionIndex) {
                bool actionSelected = (actionIndex == m_selectedActionIndex);
                RenderActionItem(currentLine + 1 + actionIndex, actions[actionIndex], actionSelected);
            }

            currentLine += actions.size();
        }

        currentLine++;
    }

    if (!m_showActionsForSlot) {
        int backLine = 6 + TOTAL_SLOTS + 1;
        bool backSelected = (m_selectedSlot == TOTAL_SLOTS + 1);

        RenderActionItem(backLine, "Back", backSelected);
    }
}

void SaveSelectionMenu::RenderTemplatesList() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    if (m_needFullRedraw) {
        SetConsoleTextAttribute(hConsole, 14);
        rlutil::locate(2, 1);
        std::cout << "Select Template for Slot " << m_templateForSlot;

        SetConsoleTextAttribute(hConsole, 7);
        rlutil::locate(2, 3);
        std::cout << "Templates";
        rlutil::locate(2, 4);
        std::cout << "------------------------------------------------------------";
    }

    if (m_templates.empty()) {
        m_templates = m_templateSystem->GetTemplates();
    }

    const int START_LINE = 6;
    const int ROWS = 6;
    const int COLUMNS = 5;
    const int TOTAL_TEMPLATES = 30;

    if (m_needFullRedraw) {
        for (int i = START_LINE; i <= START_LINE + ROWS + 2; i++) {
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
            int yPos = START_LINE + row;

            bool isSelected = (m_selectedTemplateIndex + 1 == slotNumber);
            bool wasSelected = (m_prevSelectedTemplateIndex + 1 == slotNumber);

            if (m_needFullRedraw || isSelected != wasSelected) {
                rlutil::locate(xPos, yPos);
                for (int j = 0; j < 15; j++) {
                    std::cout << ' ';
                }
                rlutil::locate(xPos, yPos);

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

    int backLine = START_LINE + ROWS + 1;
    bool backSelected = (m_selectedTemplateIndex == TOTAL_TEMPLATES);
    bool backWasSelected = m_prevBackSelected;

    if (m_needFullRedraw || backSelected != backWasSelected) {
        rlutil::locate(4, backLine);
        for (int j = 0; j < 15; j++) {
            std::cout << ' ';
        }
        rlutil::locate(4, backLine);

        if (backSelected) {
            SetConsoleTextAttribute(hConsole, 10);
            std::cout << "> Back";
        }
        else {
            SetConsoleTextAttribute(hConsole, 7);
            std::cout << "  Back";
        }
        SetConsoleTextAttribute(hConsole, 7);
    }

    m_prevSelectedTemplateIndex = m_selectedTemplateIndex;
    m_prevBackSelected = backSelected;
}

void SaveSelectionMenu::RenderSaveItem(int line, const SaveInfo& save, bool selected) {
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

    std::cout << save.slotNumber << ". " << save.GetDisplayName();

    SetConsoleTextAttribute(hConsole, 7);
}

void SaveSelectionMenu::RenderTemplateItem(int line, const TemplateInfo& templateInfo, bool selected) {
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

void SaveSelectionMenu::RenderActionItem(int line, const std::string& text, bool selected) {
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

void SaveSelectionMenu::ProcessInput() {
    if (!m_inputManager) return;

    if (m_currentState == SaveActionState::WORLD_EDITOR && m_worldEditor) {
        m_worldEditor->ProcessInput();

        if (m_worldEditor->ShouldReturnToSaves()) {
            m_currentState = SaveActionState::MAIN_LIST;
            m_worldEditor.reset();
            m_needFullRedraw = true;
            m_showActionsForSlot = false;
            m_actionSlot = -1;
            Logger::Log("World Editor closed, returning to save selection");
        }
        else if (m_worldEditor->ShouldCreateWorld()) {
            m_currentState = SaveActionState::MAIN_LIST;
            m_worldEditor.reset();
            m_needFullRedraw = true;
            m_showActionsForSlot = false;
            m_actionSlot = -1;
            m_saves = m_saveSystem->GetAllSaves();
            Logger::Log("World created, returning to save selection");
        }
        return;
    }

    if (m_inTemplateSelection) {
        const int ROWS = 6;
        const int COLUMNS = 5;
        const int TOTAL_TEMPLATES = 30;

        int oldIndex = m_selectedTemplateIndex;

        if (m_inputManager->IsMenuUp() || m_inputManager->IsKeyPressed('W')) {
            if (m_selectedTemplateIndex == TOTAL_TEMPLATES) {
                m_selectedTemplateIndex = 5;
            }
            else if (m_selectedTemplateIndex < TOTAL_TEMPLATES) {
                int currentRow = m_selectedTemplateIndex % ROWS;
                int currentCol = m_selectedTemplateIndex / ROWS;

                if (currentRow > 0) {
                    m_selectedTemplateIndex--;
                }
                else {
                    if (currentCol > 0) {
                        m_selectedTemplateIndex = (currentCol - 1) * ROWS + (ROWS - 1);
                    }
                    else {
                        m_selectedTemplateIndex = TOTAL_TEMPLATES;
                    }
                }
            }
            Logger::Log("Template selection: Up -> Index " + std::to_string(m_selectedTemplateIndex));
        }
        else if (m_inputManager->IsMenuDown() || m_inputManager->IsKeyPressed('S')) {
            if (m_selectedTemplateIndex == TOTAL_TEMPLATES) {
                m_selectedTemplateIndex = 0;
            }
            else if (m_selectedTemplateIndex < TOTAL_TEMPLATES) {
                int currentRow = m_selectedTemplateIndex % ROWS;
                int currentCol = m_selectedTemplateIndex / ROWS;

                if (currentRow < ROWS - 1) {
                    m_selectedTemplateIndex++;

                    if (m_selectedTemplateIndex >= TOTAL_TEMPLATES) {
                        m_selectedTemplateIndex = TOTAL_TEMPLATES;
                    }
                }
                else {
                    if (currentCol < COLUMNS - 1) {
                        int newIndex = (currentCol + 1) * ROWS;
                        if (newIndex < TOTAL_TEMPLATES) {
                            m_selectedTemplateIndex = newIndex;
                        }
                        else {
                            m_selectedTemplateIndex = TOTAL_TEMPLATES;
                        }
                    }
                    else {
                        m_selectedTemplateIndex = TOTAL_TEMPLATES;
                    }
                }
            }
            Logger::Log("Template selection: Down -> Index " + std::to_string(m_selectedTemplateIndex));
        }
        else if (m_inputManager->IsKeyPressed('A') || m_inputManager->IsKeyPressed(VK_LEFT)) {
            if (m_selectedTemplateIndex < TOTAL_TEMPLATES) {
                int currentRow = m_selectedTemplateIndex % ROWS;
                int currentCol = m_selectedTemplateIndex / ROWS;

                if (currentCol > 0) {
                    m_selectedTemplateIndex = (currentCol - 1) * ROWS + currentRow;
                }
                else {
                    m_selectedTemplateIndex = (COLUMNS - 1) * ROWS + currentRow;

                    if (m_selectedTemplateIndex >= TOTAL_TEMPLATES) {
                        while (m_selectedTemplateIndex >= TOTAL_TEMPLATES && currentRow > 0) {
                            currentRow--;
                            m_selectedTemplateIndex = (COLUMNS - 1) * ROWS + currentRow;
                        }
                        if (m_selectedTemplateIndex >= TOTAL_TEMPLATES) {
                            m_selectedTemplateIndex = TOTAL_TEMPLATES;
                        }
                    }
                }
            }
            Logger::Log("Template selection: Left -> Index " + std::to_string(m_selectedTemplateIndex));
        }
        else if (m_inputManager->IsKeyPressed('D') || m_inputManager->IsKeyPressed(VK_RIGHT)) {
            if (m_selectedTemplateIndex < TOTAL_TEMPLATES) {
                int currentRow = m_selectedTemplateIndex % ROWS;
                int currentCol = m_selectedTemplateIndex / ROWS;

                if (currentCol < COLUMNS - 1) {
                    int newIndex = (currentCol + 1) * ROWS + currentRow;
                    if (newIndex < TOTAL_TEMPLATES) {
                        m_selectedTemplateIndex = newIndex;
                    }
                    else {
                        m_selectedTemplateIndex = TOTAL_TEMPLATES;
                    }
                }
                else {
                    m_selectedTemplateIndex = currentRow;
                }
            }
            Logger::Log("Template selection: Right -> Index " + std::to_string(m_selectedTemplateIndex));
        }
        else if (m_inputManager->IsMenuSelect()) {
            if (m_selectedTemplateIndex == TOTAL_TEMPLATES) {
                m_inTemplateSelection = false;
                m_templateForSlot = -1;
                m_selectedTemplateIndex = 0;
                m_needFullRedraw = true;
                Logger::Log("Back from template selection");
            }
            else {
                SelectTemplateForSave(m_selectedTemplateIndex + 1);
            }
        }
        else if (m_inputManager->IsMenuBack()) {
            m_inTemplateSelection = false;
            m_templateForSlot = -1;
            m_selectedTemplateIndex = 0;
            m_prevSelectedTemplateIndex = -1;
            m_prevBackSelected = false;
            m_needFullRedraw = true;
            Logger::Log("Back from template selection (ESC)");
        }

        if (oldIndex != m_selectedTemplateIndex) {
            Logger::Log("Template index changed from " + std::to_string(oldIndex) +
                " to " + std::to_string(m_selectedTemplateIndex));
        }

        return;
    }

    if (m_inputManager->IsMenuUp() || m_inputManager->IsKeyPressed('W')) {
        Logger::Log("SaveSelectionMenu MenuUp or W pressed");
        SelectPreviousOption();
    }
    else if (m_inputManager->IsMenuDown() || m_inputManager->IsKeyPressed('S')) {
        Logger::Log("SaveSelectionMenu MenuDown or S pressed");
        SelectNextOption();
    }
    else if (m_inputManager->IsMenuSelect()) {
        Logger::Log("SaveSelectionMenu MenuSelect pressed - Current state: " +
            std::to_string(static_cast<int>(m_currentState)));
        ConfirmSelection();
    }
    else if (m_inputManager->IsMenuBack()) {
        Logger::Log("SaveSelectionMenu MenuBack pressed");
        if (m_showActionsForSlot) {
            m_showActionsForSlot = false;
            m_actionSlot = -1;
            m_selectedActionIndex = 0;
            m_needFullRedraw = true;
            Logger::Log("SaveSelectionMenu Back from actions to MAIN_LIST");
        }
        else {
            m_shouldReturn = true;
            Logger::Log("SaveSelectionMenu Returning to main menu");
        }
    }
}

void SaveSelectionMenu::SelectNextOption() {
    if (m_showActionsForSlot) {
        SaveInfo selectedSave;
        for (const auto& save : m_saves) {
            if (save.slotNumber == m_actionSlot) {
                selectedSave = save;
                break;
            }
        }
        if (selectedSave.slotNumber != m_actionSlot) {
            selectedSave.slotNumber = m_actionSlot;
            selectedSave.isEmpty = true;
        }
        const auto& actions = selectedSave.isEmpty ? m_emptySaveActions : m_usedSaveActions;
        m_selectedActionIndex = (m_selectedActionIndex + 1) % actions.size();
    }
    else {
        const int TOTAL_SLOTS = 5;
        int oldSlot = m_selectedSlot;
        m_selectedSlot = (m_selectedSlot % (TOTAL_SLOTS + 1)) + 1;
        Logger::Log("SaveSelectionMenu SelectNext - Slot: " + std::to_string(oldSlot) +
            " -> " + std::to_string(m_selectedSlot));
    }
}

void SaveSelectionMenu::SelectPreviousOption() {
    if (m_showActionsForSlot) {
        SaveInfo selectedSave;
        for (const auto& save : m_saves) {
            if (save.slotNumber == m_actionSlot) {
                selectedSave = save;
                break;
            }
        }
        if (selectedSave.slotNumber != m_actionSlot) {
            selectedSave.slotNumber = m_actionSlot;
            selectedSave.isEmpty = true;
        }
        const auto& actions = selectedSave.isEmpty ? m_emptySaveActions : m_usedSaveActions;
        m_selectedActionIndex = (m_selectedActionIndex - 1 + actions.size()) % actions.size();
    }
    else {
        const int TOTAL_SLOTS = 5;
        int oldSlot = m_selectedSlot;

        if (m_selectedSlot == 1) {
            m_selectedSlot = TOTAL_SLOTS + 1;
        }
        else if (m_selectedSlot == TOTAL_SLOTS + 1) {
            m_selectedSlot = TOTAL_SLOTS;
        }
        else {
            m_selectedSlot = m_selectedSlot - 1;
        }

        Logger::Log("SaveSelectionMenu SelectPrevious - Slot: " + std::to_string(oldSlot) +
            " -> " + std::to_string(m_selectedSlot));
    }
}


void SaveSelectionMenu::ConfirmSelection() {
    Logger::Log("SaveSelectionMenu ConfirmSelection - State: " +
        std::to_string(static_cast<int>(m_currentState)) +
        ", Slot: " + std::to_string(m_selectedSlot) +
        ", ShowActions: " + std::to_string(m_showActionsForSlot));

    if (m_showActionsForSlot) {
        SaveInfo selectedSave;
        for (const auto& save : m_saves) {
            if (save.slotNumber == m_actionSlot) {
                selectedSave = save;
                break;
            }
        }

        if (selectedSave.slotNumber != m_actionSlot) {
            selectedSave.slotNumber = m_actionSlot;
            selectedSave.isEmpty = true;
        }

        const auto& actions = selectedSave.isEmpty ? m_emptySaveActions : m_usedSaveActions;

        if (m_selectedActionIndex >= actions.size()) {
            Logger::Log("SaveSelectionMenu ERROR: Action index out of bounds");
            return;
        }

        std::string selectedAction = actions[m_selectedActionIndex];
        Logger::Log("SaveSelectionMenu Action selected: " + selectedAction + " for slot " +
            std::to_string(m_actionSlot));

        if (selectedAction == "Cancel" || selectedAction == "Back") {
            m_showActionsForSlot = false;
            m_actionSlot = -1;
            m_selectedActionIndex = 0;
            m_needFullRedraw = true;
            Logger::Log("Cancelled actions for slot " + std::to_string(m_actionSlot));
        }
        else if (selectedAction == "Create") {
            ShowCreateOptionsForSlot(m_actionSlot);
        }
        else if (selectedAction == "Play") {
            Logger::Log("SaveSelectionMenu Playing save slot " + std::to_string(m_actionSlot));

            m_saveSystem->SetSelectedSlot(m_actionSlot);

            if (selectedSave.isEmpty) {
                Logger::Log("ERROR: Cannot play empty save slot " + std::to_string(m_actionSlot));
            }
            else if (m_saveSystem->LoadSave(selectedSave.gameMode, m_actionSlot)) {
                m_shouldStartGame = true;
                m_gameMode = selectedSave.gameMode;
                Logger::Log("Save loaded successfully, starting game...");
            }
            else {
                Logger::Log("ERROR: Failed to load save slot " + std::to_string(m_actionSlot));
            }
        }
        else if (selectedAction == "Create from template") {
            ShowTemplatesForSlot(m_actionSlot);
        }
        else if (selectedAction == "Create custom world") {
            m_worldEditor = std::make_unique<WorldEditor>(
                EditorMode::CREATE_WORLD,
                m_actionSlot,
                GameMode::PROCEDURAL_GENERATION
            );
            m_worldEditor->Initialize();
            m_currentState = SaveActionState::WORLD_EDITOR;
            rlutil::cls();
            m_needFullRedraw = true;
            m_showActionsForSlot = false;
            m_actionSlot = -1;
            Logger::Log("Opening World Editor for slot " + std::to_string(m_actionSlot));
        }
        else if (selectedAction == "Delete") {
            Logger::Log("SaveSelectionMenu Deleting save slot " + std::to_string(m_actionSlot));

            if (selectedSave.isEmpty) {
                Logger::Log("Slot " + std::to_string(m_actionSlot) + " is already empty");
                return;
            }

            if (m_saveSystem->DeleteSave(selectedSave.gameMode, m_actionSlot)) {
                Logger::Log("Successfully deleted save slot " + std::to_string(m_actionSlot));
                std::vector<SaveInfo> existingSaves = m_saveSystem->GetAllSaves();
                m_saves.clear();

                const int TOTAL_SLOTS = 5;
                for (int slot = 1; slot <= TOTAL_SLOTS; slot++) {
                    bool slotExists = false;
                    SaveInfo slotSave;

                    for (const auto& save : existingSaves) {
                        if (save.slotNumber == slot) {
                            slotExists = true;
                            slotSave = save;
                            break;
                        }
                    }

                    if (slotExists) {
                        m_saves.push_back(slotSave);
                    }
                    else {
                        SaveInfo emptySave;
                        emptySave.slotNumber = slot;
                        emptySave.name = "";
                        emptySave.gameMode = GameMode::PROCEDURAL_GENERATION;
                        emptySave.isEmpty = true;
                        emptySave.creationDate = "";
                        emptySave.lastPlayedDate = "";
                        emptySave.savePath = "";
                        emptySave.templateId = -1;
                        m_saves.push_back(emptySave);
                    }
                }

                m_prevSaves = m_saves;

                m_showActionsForSlot = false;
                m_actionSlot = -1;
                m_needFullRedraw = true;

                Logger::Log("Save deleted, returning to saves list");
            }
            else {
                Logger::Log("ERROR: Failed to delete save slot " + std::to_string(m_actionSlot));
            }
        }
    }
    else {
        const int TOTAL_SLOTS = 5;

        if (m_selectedSlot == TOTAL_SLOTS + 1) {
            Logger::Log("SaveSelectionMenu Back selected - returning to main menu");
            m_shouldReturn = true;
            return;
        }

        SaveInfo selectedSave;
        for (const auto& save : m_saves) {
            if (save.slotNumber == m_selectedSlot) {
                selectedSave = save;
                break;
            }
        }

        if (selectedSave.slotNumber != m_selectedSlot) {
            selectedSave.slotNumber = m_selectedSlot;
            selectedSave.isEmpty = true;
        }

        m_showActionsForSlot = true;
        m_actionSlot = m_selectedSlot;
        m_selectedActionIndex = 0;
        m_needFullRedraw = true;
        Logger::Log("Showing actions for slot " + std::to_string(m_actionSlot) +
            " (" + (selectedSave.isEmpty ? "Empty" : "Used") + ")");
    }
}

void SaveSelectionMenu::ShowCreateOptionsForSlot(int slot) {
    std::vector<std::string> createOptions = {
        "Create from template",
        "Create custom world",
        "Cancel"
    };

    bool prevShowActions = m_showActionsForSlot;
    int prevActionSlot = m_actionSlot;
    int prevActionIndex = m_selectedActionIndex;

    m_showActionsForSlot = true;
    m_actionSlot = slot;
    m_selectedActionIndex = 0;
    m_needFullRedraw = true;

    std::vector<std::string> tempEmptyActions = m_emptySaveActions;
    m_emptySaveActions = createOptions;

    Render();

    bool choiceMade = false;
    while (!choiceMade) {
        ProcessInput();

        if (!m_showActionsForSlot || m_actionSlot == -1) {
            choiceMade = true;
        }
        else if (m_inputManager->IsMenuSelect()) {
            if (m_selectedActionIndex == 0) { // Create from template
                ShowTemplatesForSlot(slot);
                choiceMade = true;
            }
            else if (m_selectedActionIndex == 1) { // Create custom world
                m_worldEditor = std::make_unique<WorldEditor>(
                    EditorMode::CREATE_WORLD,
                    slot,
                    GameMode::PROCEDURAL_GENERATION
                );
                m_worldEditor->Initialize();
                m_currentState = SaveActionState::WORLD_EDITOR;
                rlutil::cls();
                m_needFullRedraw = true;
                m_showActionsForSlot = false;
                m_actionSlot = -1;
                Logger::Log("Opening World Editor for slot " + std::to_string(slot));
                choiceMade = true;
            }
            else if (m_selectedActionIndex == 2) { // Cancel
                m_showActionsForSlot = false;
                m_actionSlot = -1;
                m_selectedActionIndex = 0;
                m_needFullRedraw = true;
                choiceMade = true;
            }
        }
        else if (m_inputManager->IsMenuBack()) {
            m_showActionsForSlot = false;
            m_actionSlot = -1;
            m_selectedActionIndex = 0;
            m_needFullRedraw = true;
            choiceMade = true;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    m_emptySaveActions = tempEmptyActions;

    if (!choiceMade) {
        m_showActionsForSlot = prevShowActions;
        m_actionSlot = prevActionSlot;
        m_selectedActionIndex = prevActionIndex;
    }

    m_needFullRedraw = true;
}

void SaveSelectionMenu::ShowTemplatesForSlot(int slot) {
    m_templateForSlot = slot;
    m_inTemplateSelection = true;
    m_selectedTemplateIndex = 0;
    m_prevSelectedTemplateIndex = -1;
    m_prevBackSelected = false;
    m_templates = m_templateSystem->GetTemplates();
    m_needFullRedraw = true;

    Logger::Log("Showing templates for slot " + std::to_string(slot));
}


void SaveSelectionMenu::SelectTemplateForSave(int templateSlot) {
    if (templateSlot < 1 || templateSlot > TemplateSystem::MAX_TEMPLATES) {
        Logger::Log("ERROR: Invalid template slot: " + std::to_string(templateSlot));
        return;
    }

    WorldConfig templateConfig;
    if (!m_templateSystem->LoadTemplate(templateSlot, templateConfig)) {
        Logger::Log("ERROR: Failed to load template " + std::to_string(templateSlot));
        return;
    }

    // Важно: помечаем, что конфиг загружен из шаблона
    templateConfig.MarkAsLoadedFromSave();

    std::string saveName = templateConfig.GetWorldName() + " (from template)";

    // Сначала создаем базовый сейв с world_gen.cfg
    if (m_saveSystem->CreateNewSave(GameMode::PROCEDURAL_GENERATION, m_templateForSlot, saveName, templateConfig)) {
        // Копируем ВСЕ файлы из шаблона
        if (!m_saveSystem->CopyTemplateToSave(templateSlot, m_templateForSlot, GameMode::PROCEDURAL_GENERATION)) {
            Logger::Log("WARNING: Failed to copy all files from template");
        }

        // ТЕПЕРЬ СОЗДАЕМ WORLD EDITOR С КОНФИГОМ ШАБЛОНА
        // Это важно для инициализации правильных путей к еде
        m_worldEditor = std::make_unique<WorldEditor>(
            EditorMode::CREATE_WORLD,
            m_templateForSlot,
            GameMode::PROCEDURAL_GENERATION
        );

        // Загружаем конфигурацию из шаблона в редактор
        m_worldEditor->LoadTemplateConfig(templateConfig);

        // Теперь сохраняем конфигурацию через редактор
        m_worldEditor->SaveWorldConfiguration();

        // Сохраняем все конфигурации (включая food.cfg)
        std::string savePath = m_saveSystem->GetSaveSlotPath(GameMode::PROCEDURAL_GENERATION, m_templateForSlot);
        m_worldEditor->SaveAllConfigurations(savePath);

        Logger::Log("Created save from template " + std::to_string(templateSlot) +
            " in slot " + std::to_string(m_templateForSlot));

        m_saves = m_saveSystem->GetAllSaves();
        m_needFullRedraw = true;

        m_inTemplateSelection = false;
        m_templateForSlot = -1;
        m_selectedTemplateIndex = 0;

        Logger::Log("Save created from template successfully");
    }
    else {
        Logger::Log("ERROR: Failed to create save from template");
    }
}

void SaveSelectionMenu::ClearMenuArea() {
    for (int line = 3; line <= 20; line++) {
        ClearLine(line);
    }
}

void SaveSelectionMenu::ClearLine(int line) {
    rlutil::locate(0, line);
    for (int i = 0; i < 80; i++) {
        std::cout << ' ';
    }
}

void SaveSelectionMenu::Reset() {
    Logger::Log("SaveSelectionMenu Reset() called");

    m_selectedSlot = 1;
    m_selectedActionIndex = 0;
    m_shouldReturn = false;
    m_shouldStartGame = false;
    m_currentState = SaveActionState::MAIN_LIST;
    m_needFullRedraw = true;
    m_showActionsForSlot = false;
    m_actionSlot = -1;
    m_selectedTemplateIndex = 0;
    m_templateForSlot = -1;
    m_inTemplateSelection = false;

    std::vector<SaveInfo> existingSaves = m_saveSystem->GetAllSaves();
    m_saves.clear();

    const int TOTAL_SLOTS = 5;
    for (int slot = 1; slot <= TOTAL_SLOTS; slot++) {
        bool slotExists = false;
        SaveInfo slotSave;

        for (const auto& save : existingSaves) {
            if (save.slotNumber == slot) {
                slotExists = true;
                slotSave = save;
                break;
            }
        }

        if (slotExists) {
            m_saves.push_back(slotSave);
        }
        else {
            SaveInfo emptySave;
            emptySave.slotNumber = slot;
            emptySave.name = "";
            emptySave.gameMode = GameMode::PROCEDURAL_GENERATION;
            emptySave.isEmpty = true;
            emptySave.creationDate = "";
            emptySave.lastPlayedDate = "";
            emptySave.savePath = "";
            emptySave.templateId = -1;
            m_saves.push_back(emptySave);
        }
    }

    m_prevSaves = m_saves;

    Logger::Log("SaveSelectionMenu Reset completed");
}

void SaveSelectionMenu::UpdateHelpForCurrentSelection() {
    auto& helpSystem = HelpSystem::GetInstance();
    std::string currentItemId;

    if (m_showActionsForSlot) {
        SaveInfo selectedSave;
        for (const auto& save : m_saves) {
            if (save.slotNumber == m_actionSlot) {
                selectedSave = save;
                break;
            }
        }

        if (selectedSave.isEmpty) {
            switch (m_selectedActionIndex) {
            case 0: currentItemId = "Create"; break;
            case 1: currentItemId = "Back"; break;
            }
        }
        else {
            switch (m_selectedActionIndex) {
            case 0: currentItemId = "Play"; break;
            case 1: currentItemId = "Delete"; break;
            case 2: currentItemId = "Back"; break;
            }
        }
    }
    else {
        const int TOTAL_SLOTS = 5;
        if (m_selectedSlot <= TOTAL_SLOTS) {
            currentItemId = "save_slot_" + std::to_string(m_selectedSlot);
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