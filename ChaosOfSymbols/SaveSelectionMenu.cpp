#include "SaveSelectionMenu.h"
#include <windows.h>
#include <iostream>
#include <chrono>
#include <thread>
#include "Logger.h"

namespace rlutil {
    void setColor(int color);
    void cls();
    void locate(int x, int y);
    void hideCursor();
}

SaveSelectionMenu::SaveSelectionMenu(GameMode mode)
    : m_gameMode(mode)
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
{
    Logger::Log("=== SaveSelectionMenu CONSTRUCTOR ===");
    Logger::Log("Mode: " + std::to_string(static_cast<int>(mode)));
    Logger::Log("Initial state: " + std::to_string(static_cast<int>(m_currentState)));
    Logger::Log("Selected slot: " + std::to_string(m_selectedSlot));

    m_saveSystem = std::make_unique<SaveSystem>();
    m_saves = m_saveSystem->GetSaves(mode);
    m_prevSaves = m_saves;
    m_inputManager = std::make_unique<InputManager>();

    m_emptySaveActions = {
        "Create",
        "Back"
    };


    m_usedSaveActions = {
        "Load",
        "Delete",
        "Back"
    };

    Logger::Log("Constructor completed. Saves count: " + std::to_string(m_saves.size()));
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

    if (m_inputManager) {
        m_inputManager->ClearSystemBuffer();
        m_inputManager->ClearState();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    Logger::Log("SaveSelectionMenu Initialize() - State reset to MAIN_LIST");
    Logger::Log("Current state after init: " + std::to_string(static_cast<int>(m_currentState)));
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
        Logger::Log("SaveSelectionMenu State changed from " + std::to_string(static_cast<int>(m_prevState)) +
            " to " + std::to_string(static_cast<int>(m_currentState)));
        rlutil::cls();
        m_needFullRedraw = true;
        m_prevState = m_currentState;
    }

    if (m_needFullRedraw) {
        rlutil::cls();
    }

    RenderOnlyChanges();

    m_prevSelectedSlot = m_selectedSlot;
    m_prevSelectedActionIndex = m_selectedActionIndex;
    m_prevSaves = m_saves;
    m_needFullRedraw = false;
}

bool SaveSelectionMenu::NeedsRedraw() const {
    return m_prevSelectedSlot != m_selectedSlot ||
        m_prevSelectedActionIndex != m_selectedActionIndex ||
        m_currentState != m_prevState ||
        m_showActionsForSlot != (m_actionSlot != -1);
}

void SaveSelectionMenu::RenderOnlyChanges() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    if (m_needFullRedraw) {
        SetConsoleTextAttribute(hConsole, 14);
        rlutil::locate(2, 1);
        std::cout << (m_gameMode == GameMode::PROCEDURAL_GENERATION ?
            "Procedural Generation Saves" : "Preloaded Maps Saves");

        SetConsoleTextAttribute(hConsole, 7);
        rlutil::locate(2, 3);
        std::cout << "Saves";
        rlutil::locate(2, 4);
        std::cout << "------------------------------------------------------------";
    }

    RenderSavesList();

    if (m_needFullRedraw) {
        rlutil::locate(2, 22);
        std::cout << "Controls: W/S - Navigate, SPACE/ENTER - Select, Q/ESC - Back";
    }

    SetConsoleTextAttribute(hConsole, 7);
}

void SaveSelectionMenu::RenderSavesList() {
    int currentLine = 6;

    for (size_t i = 0; i < m_saves.size(); ++i) {
        const auto& save = m_saves[i];
        bool isSelected = (save.slotNumber == m_selectedSlot && !m_showActionsForSlot);
        bool wasSelected = (m_prevSaves.size() > i && m_prevSaves[i].slotNumber == m_prevSelectedSlot && !m_showActionsForSlot);

        if (m_needFullRedraw || isSelected != wasSelected) {
            RenderSaveItem(currentLine, save, isSelected);
        }

        if (m_showActionsForSlot && save.slotNumber == m_actionSlot) {
            const auto& actions = save.isEmpty ? m_emptySaveActions : m_usedSaveActions;
            for (size_t actionIndex = 0; actionIndex < actions.size(); ++actionIndex) {
                bool actionSelected = (actionIndex == m_selectedActionIndex);
                bool prevActionSelected = (actionIndex == m_prevSelectedActionIndex);

                if (m_needFullRedraw || actionSelected != prevActionSelected) {
                    RenderActionItem(currentLine + 1 + actionIndex, actions[actionIndex], actionSelected);
                }
            }
        }

        currentLine++;
        if (m_showActionsForSlot && save.slotNumber == m_actionSlot) {
            currentLine += (save.isEmpty ? m_emptySaveActions.size() : m_usedSaveActions.size());
        }
    }

    if (!m_showActionsForSlot) {
        int backLine = 6 + m_saves.size() + 1;
        bool backSelected = (m_selectedSlot == 6);

        if (m_needFullRedraw || backSelected != (m_prevSelectedSlot == 6)) {
            RenderActionItem(backLine, "Back", backSelected);
        }
    }
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

    std::cout << save.slotNumber << ". " << save.name;

    if (!save.isEmpty) {
        std::cout << " (" << save.creationDate;
        if (!save.lastPlayedDate.empty()) {
            std::cout << " - " << save.lastPlayedDate;
        }
        std::cout << ")";
    }

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
            m_saves = m_saveSystem->GetSaves(m_gameMode);
            Logger::Log("World created, returning to save selection");
        }
        return;
    }

    if (m_inputManager->IsMenuUp()) {
        Logger::Log("SaveSelectionMenu MenuUp pressed");
        SelectPreviousOption();
    }
    else if (m_inputManager->IsMenuDown()) {
        Logger::Log("SaveSelectionMenu MenuDown pressed");
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
        const auto& actions = selectedSave.isEmpty ? m_emptySaveActions : m_usedSaveActions;
        m_selectedActionIndex = (m_selectedActionIndex + 1) % actions.size();
    }
    else {
        int oldSlot = m_selectedSlot;
        m_selectedSlot = (m_selectedSlot % 6) + 1;
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
        const auto& actions = selectedSave.isEmpty ? m_emptySaveActions : m_usedSaveActions;
        m_selectedActionIndex = (m_selectedActionIndex - 1 + actions.size()) % actions.size();
    }
    else {
        int oldSlot = m_selectedSlot;
        m_selectedSlot = (m_selectedSlot - 2 + 6) % 6 + 1;
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

        const auto& actions = selectedSave.isEmpty ? m_emptySaveActions : m_usedSaveActions;

        if (m_selectedActionIndex >= actions.size()) {
            Logger::Log("SaveSelectionMenu ERROR: Action index out of bounds");
            return;
        }

        std::string selectedAction = actions[m_selectedActionIndex];
        Logger::Log("SaveSelectionMenu Action selected: " + selectedAction + " for slot " +
            std::to_string(m_actionSlot));

        if (selectedAction == "Back") {
            m_showActionsForSlot = false;
            m_actionSlot = -1;
            m_selectedActionIndex = 0;
            m_needFullRedraw = true;
        }
        else if (selectedAction == "Load") {
            Logger::Log("SaveSelectionMenu Loading save slot " + std::to_string(m_actionSlot));

            m_saveSystem->SetSelectedSlot(m_actionSlot);

            if (m_saveSystem->LoadSave(m_gameMode, m_actionSlot)) {
                m_shouldStartGame = true;
                Logger::Log("Save loaded successfully, starting game...");
            }
            else {
                Logger::Log("ERROR: Failed to load save slot " + std::to_string(m_actionSlot));
            }
        }
        else if (selectedAction == "Create") {
            m_worldEditor = std::make_unique<WorldEditor>(m_gameMode, m_actionSlot);
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

            if (m_saveSystem->DeleteSave(m_gameMode, m_actionSlot)) {
                Logger::Log("Successfully deleted save slot " + std::to_string(m_actionSlot));
                m_saves = m_saveSystem->GetSaves(m_gameMode);
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
        if (m_selectedSlot == 6) {
            Logger::Log("SaveSelectionMenu Back selected - returning to main menu");
            m_shouldReturn = true;
            return;
        }

        m_showActionsForSlot = true;
        m_actionSlot = m_selectedSlot;
        m_selectedActionIndex = 0;
        m_needFullRedraw = true;
        Logger::Log("Showing actions for slot " + std::to_string(m_actionSlot));
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

    m_saves = m_saveSystem->GetSaves(m_gameMode);
    m_prevSaves = m_saves;

    Logger::Log("SaveSelectionMenu Reset completed - State: " +
        std::to_string(static_cast<int>(m_currentState)) +
        ", Slot: " + std::to_string(m_selectedSlot));
}