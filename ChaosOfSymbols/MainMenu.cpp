#include "MainMenu.h"
#include <windows.h>
#include <chrono>
#include <iostream>
#include "Logger.h"
#include "HelpPanel.h"
#include "HelpSystem.h"

namespace rlutil {
    void setColor(int color);
    void cls();
    void locate(int x, int y);
    void hideCursor();
}

MainMenu::MainMenu()
    : m_currentState(MenuState::MAIN_MENU)
    , m_selectedMainIndex(0)
    , m_selectedSubIndex(0)
    , m_inPlaySubmenu(false)
    , m_shouldStartGame(false)
    , m_shouldExit(false)
    , m_shouldLoadSave(false)
    , m_selectedGameMode(MainMenuOption::PLAY_PROCEDURAL)
    , m_prevSelectedMainIndex(-1)
    , m_prevSelectedSubIndex(-1)
    , m_prevInPlaySubmenu(false)
    , m_prevState(MenuState::MAIN_MENU)
    , m_needFullRedraw(true)
    , m_inSaveSelection(false)
    , m_selectedSaveGameMode(GameMode::PROCEDURAL_GENERATION)
    , m_selectedSaveSlot(1)
    , m_inTemplateSelection(false)
{
    m_inputManager = std::make_unique<InputManager>();

    m_mainMenuOptions = {
        "Play",
        "About the Game",
        "Exit"
    };

    m_playSubOptions = {
        "Select save",
        "Create a world template",
        "Back"
    };

    HelpPanel::Initialize();
    auto& helpSystem = HelpSystem::GetInstance();
    helpSystem.RegisterMainMenuHelp();
    helpSystem.RegisterSaveMenuHelp();
    helpSystem.RegisterTemplateMenuHelp();
}

void MainMenu::Initialize() {
    rlutil::hideCursor();
    if (m_inputManager) {
        m_inputManager->ClearSystemBuffer();
        m_inputManager->ClearState();
    }
}

void MainMenu::Update() {
    if (m_inputManager) {
        m_inputManager->Update();
    }

    if (m_currentState == MenuState::SAVE_SELECTION && m_saveSelectionMenu) {
        m_saveSelectionMenu->Update();
        RunSaveSelection();
    }
    else if (m_currentState == MenuState::WORLD_TEMPLATE_MENU && m_worldTemplateMenu) {
        m_worldTemplateMenu->Update();
        RunWorldTemplateMenu();
    }
}

void MainMenu::Render() {
    if (m_currentState != m_prevState) {
        rlutil::cls();
        m_needFullRedraw = true;
        m_prevState = m_currentState;
    }

    switch (m_currentState) {
    case MenuState::MAIN_MENU:
        if (m_needFullRedraw || NeedsRedraw()) {
            UpdateHelpForCurrentSelection();
            RenderOnlyChanges();
        }
        break;
    case MenuState::ABOUT_SCREEN:
        if (m_needFullRedraw) {
            RenderAboutScreen();
        }
        break;
    case MenuState::SAVE_SELECTION:
        if (m_saveSelectionMenu) {
            m_saveSelectionMenu->Render();
        }
        break;
    case MenuState::WORLD_TEMPLATE_MENU:
        if (m_worldTemplateMenu) {
            m_worldTemplateMenu->Render();
        }
        break;
    case MenuState::IN_GAME:
        break;
    }

    int screenHeight = 25;
    int screenWidth = 80;
    HelpPanel::Render(screenWidth, screenHeight);

    m_prevSelectedMainIndex = m_selectedMainIndex;
    m_prevSelectedSubIndex = m_selectedSubIndex;
    m_prevInPlaySubmenu = m_inPlaySubmenu;
    m_needFullRedraw = false;
}

bool MainMenu::NeedsRedraw() const {
    return m_prevSelectedMainIndex != m_selectedMainIndex ||
        m_prevSelectedSubIndex != m_selectedSubIndex ||
        m_prevInPlaySubmenu != m_inPlaySubmenu;
}

void MainMenu::RenderOnlyChanges() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    if (m_needFullRedraw) {
        SetConsoleTextAttribute(hConsole, 4);
        rlutil::locate(2, 5);
        std::cout << "Chaos Of Symbols";
    }

    int currentLine = 8;

    bool playSelected = (m_selectedMainIndex == 0 && !m_inPlaySubmenu);
    bool prevPlaySelected = (m_prevSelectedMainIndex == 0 && !m_prevInPlaySubmenu);

    if (m_needFullRedraw || playSelected != prevPlaySelected) {
        RenderMenuItem(0, currentLine, "Play", playSelected);
    }
    currentLine++;

    if (m_inPlaySubmenu) {
        if (m_needFullRedraw || m_prevInPlaySubmenu != m_inPlaySubmenu) {
            for (int j = 0; j < m_playSubOptions.size(); ++j) {
                RenderSubMenuItem(j, currentLine + j, m_playSubOptions[j], j == m_selectedSubIndex);
            }
        }
        else {
            for (int j = 0; j < m_playSubOptions.size(); ++j) {
                bool subSelected = (j == m_selectedSubIndex);
                bool prevSubSelected = (j == m_prevSelectedSubIndex);

                if (subSelected != prevSubSelected) {
                    RenderSubMenuItem(j, currentLine + j, m_playSubOptions[j], subSelected);
                }
            }
        }
        currentLine += m_playSubOptions.size();
    }
    else {
        if (m_prevInPlaySubmenu && !m_inPlaySubmenu) {
            for (int j = 0; j < m_playSubOptions.size(); ++j) {
                ClearLine(currentLine + j);
            }
            for (int j = m_playSubOptions.size(); j < m_playSubOptions.size() + m_mainMenuOptions.size() - 1; ++j) {
                ClearLine(currentLine + j);
            }
        }
    }

    if (m_needFullRedraw || m_prevInPlaySubmenu != m_inPlaySubmenu) {
        bool aboutSelected = (m_selectedMainIndex == 1 && !m_inPlaySubmenu);
        RenderMenuItem(1, currentLine, "About the Game", aboutSelected);
    }
    else {
        bool aboutSelected = (m_selectedMainIndex == 1 && !m_inPlaySubmenu);
        bool prevAboutSelected = (m_prevSelectedMainIndex == 1 && !m_prevInPlaySubmenu);

        if (aboutSelected != prevAboutSelected) {
            RenderMenuItem(1, currentLine, "About the Game", aboutSelected);
        }
    }
    currentLine++;

    if (m_needFullRedraw || m_prevInPlaySubmenu != m_inPlaySubmenu) {
        bool exitSelected = (m_selectedMainIndex == 2 && !m_inPlaySubmenu);
        RenderMenuItem(2, currentLine, "Exit", exitSelected);
    }
    else {
        bool exitSelected = (m_selectedMainIndex == 2 && !m_inPlaySubmenu);
        bool prevExitSelected = (m_prevSelectedMainIndex == 2 && !m_prevInPlaySubmenu);

        if (exitSelected != prevExitSelected) {
            RenderMenuItem(2, currentLine, "Exit", exitSelected);
        }
    }

    SetConsoleTextAttribute(hConsole, 7);
}

void MainMenu::RenderMenuItem(int index, int line, const std::string& text, bool selected) {
    rlutil::locate(0, line);

    std::cout << "                                          ";
    rlutil::locate(0, line);

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (selected) {
        SetConsoleTextAttribute(hConsole, 10);
        std::cout << "> " << text;
    }
    else {
        SetConsoleTextAttribute(hConsole, 7);
        std::cout << "  " << text;
    }
}

void MainMenu::RenderSubMenuItem(int index, int line, const std::string& text, bool selected) {
    rlutil::locate(0, line);

    std::cout << "                                          ";
    rlutil::locate(0, line);

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (selected) {
        SetConsoleTextAttribute(hConsole, 10);
        std::cout << "  > " << text;
    }
    else {
        SetConsoleTextAttribute(hConsole, 7);
        std::cout << "    " << text;
    }
}

void MainMenu::RenderAboutScreen() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    SetConsoleTextAttribute(hConsole, 4);
    rlutil::locate(2, 5);
    std::cout << "About the Game";
    SetConsoleTextAttribute(hConsole, 7);

    rlutil::locate(2, 8);
    std::cout << "Chaos Of Symbols is a console ASCII game with a world, procedural generation, cellular automata,";
    rlutil::locate(2, 9);
    std::cout << "and flexible creation and configuration of game parameters";
    rlutil::locate(2, 11);
    std::cout << "Chaos Of Symbols by wirrwarr is licensed under MIT";
    rlutil::locate(2, 12);
    std::cout << "You are free to use and modify this asset with attribution";
    rlutil::locate(2, 14);
    std::cout << "Menu controls:";
    rlutil::locate(2, 15);
    std::cout << "W, Up Arrow - Up";
    rlutil::locate(2, 16);
    std::cout << "S, Down Arrow - Down";
    rlutil::locate(2, 17);
    std::cout << "Space, Enter - Select";
    rlutil::locate(30, 14);
    std::cout << "Game controls:";
    rlutil::locate(30, 15);
    std::cout << "WASD, Arrow keys - Move";
    rlutil::locate(30, 16);
    std::cout << "Esc - Exit to the pause menu";

    rlutil::locate(2, 20);
    SetConsoleTextAttribute(hConsole, 10);
    std::cout << "> Back";
    SetConsoleTextAttribute(hConsole, 7);
}

void MainMenu::SelectNextOption() {
    if (m_currentState == MenuState::ABOUT_SCREEN) {
        return;
    }

    if (m_inPlaySubmenu) {
        m_selectedSubIndex = (m_selectedSubIndex + 1) % m_playSubOptions.size();
    }
    else {
        m_selectedMainIndex = (m_selectedMainIndex + 1) % m_mainMenuOptions.size();
    }
}

void MainMenu::SelectPreviousOption() {
    if (m_currentState == MenuState::ABOUT_SCREEN) {
        return;
    }

    if (m_inPlaySubmenu) {
        m_selectedSubIndex = (m_selectedSubIndex - 1 + m_playSubOptions.size()) % m_playSubOptions.size();
    }
    else {
        m_selectedMainIndex = (m_selectedMainIndex - 1 + m_mainMenuOptions.size()) % m_mainMenuOptions.size();
    }
}

void MainMenu::ConfirmSelection() {
    if (m_currentState == MenuState::ABOUT_SCREEN) {
        m_currentState = MenuState::MAIN_MENU;
        m_inPlaySubmenu = false;
        m_needFullRedraw = true;
        return;
    }

    if (m_inPlaySubmenu) {
        switch (m_selectedSubIndex) {
        case 0: // Select save
            InitializeSaveSelection();
            break;
        case 1: // Create a world template
            InitializeWorldTemplateMenu();
            break;
        case 2: // back
            m_inPlaySubmenu = false;
            m_selectedSubIndex = 0;
            break;
        }
    }
    else {
        switch (m_selectedMainIndex) {
        case 0: // Play
            m_inPlaySubmenu = true;
            m_selectedSubIndex = 0;
            break;
        case 1: // About the game
            m_currentState = MenuState::ABOUT_SCREEN;
            m_inPlaySubmenu = false;
            m_needFullRedraw = true;
            break;
        case 2: // Exit
            m_shouldExit = true;
            break;
        }
    }
}

void MainMenu::InitializeSaveSelection() {
    Logger::Log("=== MainMenu::InitializeSaveSelection ===");

    m_saveSelectionMenu = std::make_unique<SaveSelectionMenu>();
    m_saveSelectionMenu->Initialize();

    m_currentState = MenuState::SAVE_SELECTION;
    m_inSaveSelection = true;
    m_needFullRedraw = true;

    Logger::Log("SaveSelectionMenu created and initialized");
}

void MainMenu::InitializeWorldTemplateMenu() {
    Logger::Log("=== MainMenu::InitializeWorldTemplateMenu ===");
    m_worldTemplateMenu = std::make_unique<WorldTemplateMenu>();
    m_worldTemplateMenu->Initialize();

    m_currentState = MenuState::WORLD_TEMPLATE_MENU;
    m_inTemplateSelection = true;
    m_needFullRedraw = true;

    Logger::Log("WorldTemplateMenu created and initialized");
    Logger::Log("MainMenu state changed to WORLD_TEMPLATE_MENU");
}

void MainMenu::RunSaveSelection() {
    if (m_saveSelectionMenu) {
        if (m_saveSelectionMenu->ShouldReturnToMainMenu()) {
            Logger::Log("MainMenu::RunSaveSelection - Returning to main menu");
            m_currentState = MenuState::MAIN_MENU;
            m_inSaveSelection = false;
            m_needFullRedraw = true;
            m_saveSelectionMenu.reset();
            Logger::Log("SaveSelectionMenu destroyed");
        }
        else if (m_saveSelectionMenu->ShouldStartGame()) {
            Logger::Log("MainMenu::RunSaveSelection - Starting game from save");
            m_shouldLoadSave = true;
            m_selectedSaveSlot = m_saveSelectionMenu->GetSelectedSlot();
            m_shouldStartGame = true;
            m_saveSelectionMenu.reset();
            Logger::Log("SaveSelectionMenu destroyed, game starting from save...");
        }
    }
}

void MainMenu::RunWorldTemplateMenu() {
    if (m_worldTemplateMenu) {
        if (m_worldTemplateMenu->ShouldReturnToMainMenu()) {
            Logger::Log("MainMenu::RunWorldTemplateMenu - Returning to main menu");
            m_currentState = MenuState::MAIN_MENU;
            m_inTemplateSelection = false;
            m_needFullRedraw = true;
            m_worldTemplateMenu.reset();
            Logger::Log("WorldTemplateMenu destroyed");
        }
        else if (m_worldTemplateMenu->ShouldCreateTemplate()) {
            Logger::Log("MainMenu::RunWorldTemplateMenu - Template created successfully");
            m_currentState = MenuState::MAIN_MENU;
            m_inTemplateSelection = false;
            m_needFullRedraw = true;
            m_worldTemplateMenu.reset();
            Logger::Log("WorldTemplateMenu destroyed, template created");
        }
    }
}

void MainMenu::ClearMenuArea() {
    for (int line = 5; line <= 20; line++) {
        ClearLine(line);
    }
}

void MainMenu::ClearLine(int line) {
    rlutil::locate(0, line);
    for (int i = 0; i < 80; i++) {
        std::cout << ' ';
    }
}

void MainMenu::ProcessInput() {
    if (!m_inputManager) return;

    if (m_currentState == MenuState::SAVE_SELECTION && m_saveSelectionMenu) {
        m_saveSelectionMenu->ProcessInput();
        return;
    }

    if (m_currentState == MenuState::WORLD_TEMPLATE_MENU && m_worldTemplateMenu) {
        m_worldTemplateMenu->ProcessInput();
        return;
    }

    if (m_currentState == MenuState::ABOUT_SCREEN) {
        if (m_inputManager->IsMenuSelect() || m_inputManager->IsMenuBack()) {
            m_currentState = MenuState::MAIN_MENU;
            m_inPlaySubmenu = false;
            m_needFullRedraw = true;
        }
        return;
    }

    if (m_inputManager->IsMenuUp()) {
        SelectPreviousOption();
    }
    else if (m_inputManager->IsMenuDown()) {
        SelectNextOption();
    }
    else if (m_inputManager->IsMenuSelect()) {
        ConfirmSelection();
    }
    else if (m_inputManager->IsMenuBack()) {
        if (m_inPlaySubmenu) {
            m_inPlaySubmenu = false;
            m_selectedSubIndex = 0;
        }
        else {
            m_shouldExit = true;
        }
    }
}

void MainMenu::UpdateHelpForCurrentSelection() {
    auto& helpSystem = HelpSystem::GetInstance();
    std::string currentItemId;

    if (m_currentState == MenuState::ABOUT_SCREEN) {
        currentItemId = "Back";
    }
    else if (m_inPlaySubmenu) {
        switch (m_selectedSubIndex) {
        case 0: currentItemId = "Select save"; break;
        case 1: currentItemId = "Create a world template"; break;
        case 2: currentItemId = "Back"; break;
        }
    }
    else {
        switch (m_selectedMainIndex) {
        case 0: currentItemId = "Play"; break;
        case 1: currentItemId = "About the Game"; break;
        case 2: currentItemId = "Exit"; break;
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