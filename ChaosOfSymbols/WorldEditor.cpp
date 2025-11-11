#include "WorldEditor.h"
#include <windows.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>
#include <sstream>
#include "Logger.h"

namespace rlutil {
    void setColor(int color);
    void cls();
    void locate(int x, int y);
    void hideCursor();
}

WorldEditor::WorldEditor(GameMode mode, int slot)
    : m_gameMode(mode)
    , m_slot(slot)
    , m_currentTab(EditorTab::WORLD)
    , m_selectedField(0)
    , m_selectedButton(0)
    , m_shouldReturn(false)
    , m_shouldCreate(false)
    , m_isEditingText(false)
    , m_editingField(-1)
    , m_prevTab(EditorTab::WORLD)
    , m_prevSelectedField(-1)
    , m_prevSelectedButton(-1)
    , m_needFullRedraw(true)
    , m_prevFieldCount(0)
{
    m_inputManager = std::make_unique<InputManager>();

    m_config.tileProbabilities['.'] = { 0.1f, 0.8f, 0.1f }; // Grass
    m_config.tileProbabilities['~'] = { 0.7f, 0.2f, 0.1f }; // Water
    m_config.tileProbabilities['^'] = { 0.1f, 0.2f, 0.7f }; // Mountain
    m_config.tileProbabilities['#'] = { 0.0f, 0.3f, 0.7f }; // Stone

    Logger::Log("WorldEditor created for slot " + std::to_string(slot));
}

void WorldEditor::Initialize() {
    rlutil::hideCursor();
    if (m_inputManager) {
        m_inputManager->ClearSystemBuffer();
        m_inputManager->ClearState();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void WorldEditor::Update() {
    if (m_inputManager) {
        m_inputManager->Update();
    }
}

void WorldEditor::Render() {
    if (m_currentTab != m_prevTab) {
        rlutil::cls();
        m_needFullRedraw = true;
        m_prevTab = m_currentTab;
        m_prevFieldCount = 0;
    }

    if (m_needFullRedraw || NeedsRedraw()) {
        RenderOnlyChanges();
    }

    m_prevSelectedField = m_selectedField;
    m_prevSelectedButton = m_selectedButton;
    m_needFullRedraw = false;
}

bool WorldEditor::NeedsRedraw() const {
    return m_prevSelectedField != m_selectedField ||
        m_prevSelectedButton != m_selectedButton ||
        m_isEditingText ||
        m_currentTab != m_prevTab; 
}

void WorldEditor::RenderOnlyChanges() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    if (m_needFullRedraw) {
        SetConsoleTextAttribute(hConsole, 14);
        rlutil::locate(2, 1);
        std::cout << "World Editor";
        SetConsoleTextAttribute(hConsole, 7);

        RenderTabHeader();

        rlutil::locate(2, 4);
        std::cout << "------------------------------------------------------------";
    }

    switch (m_currentTab) {
    case EditorTab::WORLD: RenderWorldTab(); break;
    case EditorTab::PLAYER: RenderPlayerTab(); break;
    case EditorTab::TILES: RenderTilesTab(); break;
    case EditorTab::CELLULAR_AUTOMATON: RenderCellularAutomatonTab(); break;
    case EditorTab::FOOD: RenderFoodTab(); break;
    case EditorTab::ENEMIES: RenderEnemiesTab(); break;
    case EditorTab::WIN: RenderWinTab(); break;
    case EditorTab::LOSE: RenderLoseTab(); break;
    }

    RenderBottomButtons();

    if (m_needFullRedraw) {
        ClearLine(22);
        rlutil::locate(2, 22);
        if (m_isEditingText) {
            std::cout << "Enter value and press ENTER to confirm, ESC to cancel";
        }
        else {
            std::cout << "Controls: TAB - Switch tabs, W/S - Navigate, ENTER - Edit/Select";
        }
    }

    SetConsoleTextAttribute(hConsole, 7);
}

void WorldEditor::RenderTabHeader() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    rlutil::locate(2, 3);

    std::vector<std::string> tabNames = {
        "World", "Player", "Tiles", "Cellular Automaton", "Food", "Enemies", "Win", "Lose"
    };

    for (int i = 0; i < tabNames.size(); ++i) {
        bool isSelected = (static_cast<int>(m_currentTab) == i);
        if (isSelected) {
            SetConsoleTextAttribute(hConsole, 10);
            std::cout << "[" << tabNames[i] << "] ";
        }
        else {
            SetConsoleTextAttribute(hConsole, 7);
            std::cout << tabNames[i] << " ";
        }
    }
    SetConsoleTextAttribute(hConsole, 7);
}

void WorldEditor::RenderWorldTab() {
    int line = 6;

    if (m_isEditingText && m_editingField >= 0) {
        int visibleFieldIndex = 0;
        for (int i = 0; i < 7; ++i) {
            if (i == 4 && !ShouldShowSeedField()) {
                continue;
            }

            bool isSelected = (visibleFieldIndex == m_editingField);
            if (isSelected) {
                switch (i) {
                case 0: RenderEditField(line + visibleFieldIndex, "World Name: ", m_tempStringInput, true); break;
                case 1: RenderEditField(line + visibleFieldIndex, "Width: ", m_tempStringInput, true); break;
                case 2: RenderEditField(line + visibleFieldIndex, "Height: ", m_tempStringInput, true); break;
                case 3: RenderMenuItem(line + visibleFieldIndex, "Random Generation: " + std::string(m_config.randomGeneration ? "Yes" : "No"), false); break;
                case 4: RenderEditField(line + visibleFieldIndex, "Seed: ", m_tempStringInput, true); break;
                case 5: RenderMenuItem(line + visibleFieldIndex, "Noise Frequency: " + std::to_string(m_config.noiseFrequency), false); break;
                case 6: RenderEditField(line + visibleFieldIndex, "Neighbor Radius: ", m_tempStringInput, true); break;
                }
            }
            else {
                switch (i) {
                case 0: RenderMenuItem(line + visibleFieldIndex, "World Name: " + m_config.worldName, false); break;
                case 1: RenderMenuItem(line + visibleFieldIndex, "Width: " + std::to_string(m_config.width), false); break;
                case 2: RenderMenuItem(line + visibleFieldIndex, "Height: " + std::to_string(m_config.height), false); break;
                case 3: RenderMenuItem(line + visibleFieldIndex, "Random Generation: " + std::string(m_config.randomGeneration ? "Yes" : "No"), false); break;
                case 4: RenderMenuItem(line + visibleFieldIndex, "Seed: " + std::to_string(m_config.seed), false); break;
                case 5: RenderMenuItem(line + visibleFieldIndex, "Noise Frequency: " + std::to_string(m_config.noiseFrequency), false); break;
                case 6: RenderMenuItem(line + visibleFieldIndex, "Neighbor Radius: " + std::to_string(m_config.neighborRadius), false); break;
                }
            }
            visibleFieldIndex++;
        }

        int currentFieldsCount = GetVisibleWorldFieldsCount();
        for (int i = currentFieldsCount; i < 7; ++i) {
            ClearLine(line + i);
        }
    }
    else {
        std::vector<std::string> fields;
        fields.push_back("World Name: " + m_config.worldName);
        fields.push_back("Width: " + std::to_string(m_config.width));
        fields.push_back("Height: " + std::to_string(m_config.height));
        fields.push_back("Random Generation: " + std::string(m_config.randomGeneration ? "Yes" : "No"));

        if (ShouldShowSeedField()) {
            fields.push_back("Seed: " + std::to_string(m_config.seed));
        }

        fields.push_back("Noise Frequency: " + std::to_string(m_config.noiseFrequency));
        fields.push_back("Neighbor Radius: " + std::to_string(m_config.neighborRadius));

        if (m_needFullRedraw || fields.size() != m_prevFieldCount) {
            for (int i = 0; i < fields.size(); ++i) {
                bool isSelected = (i == m_selectedField && m_selectedButton == 0);
                RenderMenuItem(line + i, fields[i], isSelected);
            }
            for (int i = fields.size(); i < m_prevFieldCount; ++i) {
                ClearLine(line + i);
            }
            m_prevFieldCount = fields.size();
        }
        else {
            for (int i = 0; i < fields.size(); ++i) {
                bool isSelected = (i == m_selectedField && m_selectedButton == 0);
                bool wasSelected = (i == m_prevSelectedField && m_prevSelectedButton == 0);

                if (isSelected != wasSelected) {
                    RenderMenuItem(line + i, fields[i], isSelected);
                }
            }
        }
    }

    int fieldsEndLine = 6 + GetVisibleWorldFieldsCount();
    int buttonsStartLine = 6 + GetMaxFields() + 2;

    for (int i = fieldsEndLine; i < buttonsStartLine; ++i) {
        ClearLine(i);
    }
}

void WorldEditor::RenderBottomButtons() {
    int line = 6 + GetMaxFields() + 2;

    for (int i = 15; i < 22; ++i) {
        ClearLine(i);
    }

    bool createSelected = (m_selectedButton == 1);
    bool backSelected = (m_selectedButton == 2);

    RenderMenuItem(line, "CREATE", createSelected);
    RenderMenuItem(line + 1, "BACK", backSelected);
}

void WorldEditor::RenderPlayerTab() {
    int line = 6;

    if (m_isEditingText && m_editingField >= 0) {
        for (int i = 0; i < 6; ++i) {
            bool isSelected = (i == m_editingField);
            if (isSelected) {
                switch (i) {
                case 0: RenderEditField(line + i, "Start X: ", m_tempStringInput, true); break;
                case 1: RenderEditField(line + i, "Start Y: ", m_tempStringInput, true); break;
                case 2: RenderEditField(line + i, "Max HP: ", m_tempStringInput, true); break;
                case 3: RenderEditField(line + i, "Max Hunger: ", m_tempStringInput, true); break;
                case 4: RenderMenuItem(line + i, "Enable HP: " + std::string(m_config.enableHP ? "Yes" : "No"), false); break;
                case 5: RenderMenuItem(line + i, "Enable Hunger: " + std::string(m_config.enableHunger ? "Yes" : "No"), false); break;
                }
            }
            else {
                switch (i) {
                case 0: RenderMenuItem(line + i, "Start X: " + std::to_string(m_config.playerStartX), false); break;
                case 1: RenderMenuItem(line + i, "Start Y: " + std::to_string(m_config.playerStartY), false); break;
                case 2: RenderMenuItem(line + i, "Max HP: " + std::to_string(m_config.playerMaxHP), false); break;
                case 3: RenderMenuItem(line + i, "Max Hunger: " + std::to_string(m_config.playerMaxHunger), false); break;
                case 4: RenderMenuItem(line + i, "Enable HP: " + std::string(m_config.enableHP ? "Yes" : "No"), false); break;
                case 5: RenderMenuItem(line + i, "Enable Hunger: " + std::string(m_config.enableHunger ? "Yes" : "No"), false); break;
                }
            }
        }
    }
    else {
        std::vector<std::string> fields = {
            "Start X: " + std::to_string(m_config.playerStartX),
            "Start Y: " + std::to_string(m_config.playerStartY),
            "Max HP: " + std::to_string(m_config.playerMaxHP),
            "Max Hunger: " + std::to_string(m_config.playerMaxHunger),
            "Enable HP: " + std::string(m_config.enableHP ? "Yes" : "No"),
            "Enable Hunger: " + std::string(m_config.enableHunger ? "Yes" : "No")
        };

        for (int i = 0; i < fields.size(); ++i) {
            bool isSelected = (i == m_selectedField && m_selectedButton == 0);
            bool wasSelected = (i == m_prevSelectedField && m_prevSelectedButton == 0);

            if (m_needFullRedraw || isSelected != wasSelected) {
                RenderMenuItem(line + i, fields[i], isSelected);
            }
        }
    }
}


void WorldEditor::RenderTilesTab() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    int line = 6;

    if (m_needFullRedraw) {
        rlutil::locate(4, line);
        std::cout << "Symbol Spawn Probabilities (Low/Mid/High zones):";
    }
    line += 2;

    int i = 0;
    for (const auto& pair : m_config.tileProbabilities) {
        char tileChar = pair.first;
        const auto& probs = pair.second;

        std::string display = std::string("'") + tileChar + "': " +
            std::to_string(probs[0]) + " / " +
            std::to_string(probs[1]) + " / " +
            std::to_string(probs[2]);

        bool isSelected = (i == m_selectedField && m_selectedButton == 0);
        bool wasSelected = (i == m_prevSelectedField && m_prevSelectedButton == 0);

        if (m_needFullRedraw || isSelected != wasSelected) {
            RenderMenuItem(line + i, display, isSelected);
        }
        i++;
    }
}

void WorldEditor::RenderCellularAutomatonTab() {
    int line = 6;

    std::vector<std::string> fields = {
        "Survival Rules: " + m_config.survivalRules,
        "Birth Rules: " + m_config.birthRules,
        "Death Rules: " + m_config.deathRules
    };

    for (int i = 0; i < fields.size(); ++i) {
        bool isSelected = (i == m_selectedField && m_selectedButton == 0);
        bool wasSelected = (i == m_prevSelectedField && m_prevSelectedButton == 0);

        if (m_needFullRedraw || isSelected != wasSelected) {
            RenderMenuItem(line + i, fields[i], isSelected);
        }
    }
}

void WorldEditor::RenderFoodTab() {
    if (m_needFullRedraw) {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        int line = 6;
        rlutil::locate(4, line);
        std::cout << "Food Spawn Settings (to be implemented)";
    }
}

void WorldEditor::RenderEnemiesTab() {
    int line = 6;

    std::vector<std::string> fields = {
        "Enable Enemies: " + std::string(m_config.enableEnemies ? "Yes" : "No"),
        "Enemy Spawn Rate: " + std::to_string(m_config.enemySpawnRate)
    };

    for (int i = 0; i < fields.size(); ++i) {
        bool isSelected = (i == m_selectedField && m_selectedButton == 0);
        bool wasSelected = (i == m_prevSelectedField && m_prevSelectedButton == 0);

        if (m_needFullRedraw || isSelected != wasSelected) {
            RenderMenuItem(line + i, fields[i], isSelected);
        }
    }
}

void WorldEditor::RenderWinTab() {
    if (m_needFullRedraw) {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        int line = 6;
        rlutil::locate(4, line);
        std::cout << "Win Conditions (to be implemented)";
    }
}

void WorldEditor::RenderLoseTab() {
    if (m_needFullRedraw) {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        int line = 6;
        rlutil::locate(4, line);
        std::cout << "Lose Conditions (to be implemented)";
    }
}

void WorldEditor::RenderMenuItem(int line, const std::string& text, bool selected) {
    rlutil::locate(4, line);

    std::cout << "                                                                        ";
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

void WorldEditor::RenderEditField(int line, const std::string& label, const std::string& value, bool selected) {
    rlutil::locate(4, line);

    std::cout << "                                                                        ";
    rlutil::locate(4, line);

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (selected) {
        SetConsoleTextAttribute(hConsole, 11);
        std::cout << "> " << label << value << "_";
    }
    else {
        SetConsoleTextAttribute(hConsole, 7);
        std::cout << "  " << label << value;
    }
    SetConsoleTextAttribute(hConsole, 7);
}

void WorldEditor::ProcessInput() {
    if (!m_inputManager) return;

    if (m_isEditingText) {
        HandleTextInput();
        return;
    }

    if (m_inputManager->IsMenuUp()) {
        SelectPreviousOption();
    }
    else if (m_inputManager->IsMenuDown()) {
        SelectNextOption();
    }
    else if (m_inputManager->IsMenuSelect()) {
        if (m_selectedButton == 0) {
            StartEditing();
        }
        else {
            ConfirmSelection();
        }
    }
    else if (m_inputManager->IsKeyPressed(VK_TAB)) {
        SelectNextTab();
    }
}

void WorldEditor::SelectNextOption() {
    if (m_selectedButton == 0) {
        if (m_selectedField < GetMaxFields() - 1) {
            m_selectedField++;
        }
        else {
            m_selectedButton = 1;
            m_selectedField = GetMaxFields() - 1;
        }
    }
    else {
        if (m_selectedButton == 1) {
            m_selectedButton = 2;
        }
        else if (m_selectedButton == 2) {
            m_selectedButton = 0;
            m_selectedField = 0;
        }
    }
}

void WorldEditor::SelectPreviousOption() {
    if (m_selectedButton == 0) {
        if (m_selectedField > 0) {
            m_selectedField--;
        }
        else {
            m_selectedButton = 2;
            m_selectedField = 0;
        }
    }
    else {
        if (m_selectedButton == 2) {
            m_selectedButton = 1;
        }
        else if (m_selectedButton == 1) {
            m_selectedButton = 0;
            m_selectedField = GetMaxFields() - 1;
        }
    }
}

void WorldEditor::StartEditing() {
    if (m_selectedButton != 0) {
        return;
    }

    m_isEditingText = true;
    m_editingField = m_selectedField;
    m_tempStringInput = "";

    switch (m_currentTab) {
    case EditorTab::WORLD:
    {
        std::vector<int> fieldMapping;
        for (int i = 0; i < 7; ++i) {
            if (i == 4 && !ShouldShowSeedField()) {
                continue;
            }
            fieldMapping.push_back(i);
        }

        if (m_editingField < fieldMapping.size()) {
            int actualField = fieldMapping[m_editingField];
            switch (actualField) {
            case 0: m_tempStringInput = m_config.worldName; break;
            case 1: m_tempStringInput = std::to_string(m_config.width); break;
            case 2: m_tempStringInput = std::to_string(m_config.height); break;
            case 3:
                m_isEditingText = false;
                m_config.randomGeneration = !m_config.randomGeneration;
                m_needFullRedraw = true;
                return;
            case 4: m_tempStringInput = std::to_string(m_config.seed); break;
            case 5: m_tempStringInput = std::to_string(m_config.noiseFrequency); break;
            case 6: m_tempStringInput = std::to_string(m_config.neighborRadius); break;
            }
        }
        break;
    }

    case EditorTab::PLAYER:
        switch (m_editingField) {
        case 0: m_tempStringInput = std::to_string(m_config.playerStartX); break;
        case 1: m_tempStringInput = std::to_string(m_config.playerStartY); break;
        case 2: m_tempStringInput = std::to_string(m_config.playerMaxHP); break;
        case 3: m_tempStringInput = std::to_string(m_config.playerMaxHunger); break;
        case 4:
            m_isEditingText = false;
            m_config.enableHP = !m_config.enableHP;
            m_needFullRedraw = true;
            return;
        case 5:
            m_isEditingText = false;
            m_config.enableHunger = !m_config.enableHunger;
            m_needFullRedraw = true;
            return;
        }
        break;
    }
}

void WorldEditor::HandleTextInput() {
    if (m_inputManager->IsKeyPressed(VK_RETURN) || m_inputManager->IsKeyPressed(VK_SPACE)) {
        ApplyEditedValue();
        m_isEditingText = false;
        m_editingField = -1;
        m_needFullRedraw = true;
        return;
    }

    if (m_inputManager->IsKeyPressed(VK_ESCAPE)) {
        m_isEditingText = false;
        m_editingField = -1;
        m_needFullRedraw = true;
        return;
    }

    switch (m_currentTab) {
    case EditorTab::WORLD:
        switch (m_editingField) {
        case 0:
            HandleTextInputGeneral();
            break;
        case 1:
        case 2:
        case 4:
        case 6:
            HandleNumericInput();
            break;
        case 3:
            HandleBooleanInput();
            break;
        case 5:
            HandleFrequencyInput();
            break;
        }
        break;

    case EditorTab::PLAYER:
        HandleNumericInput();
        break;

    default:
        HandleTextInputGeneral();
        break;
    }
}

void WorldEditor::HandleTextInputGeneral() {
    for (char c = '0'; c <= '9'; c++) {
        if (m_inputManager->IsKeyPressed(c)) {
            m_tempStringInput += c;
            m_needFullRedraw = true;
            return;
        }
    }

    for (char c = 'A'; c <= 'Z'; c++) {
        if (m_inputManager->IsKeyPressed(c)) {
            m_tempStringInput += c;
            m_needFullRedraw = true;
            return;
        }
    }

    for (char c = 'a'; c <= 'z'; c++) {
        if (m_inputManager->IsKeyPressed(c)) {
            m_tempStringInput += c;
            m_needFullRedraw = true;
            return;
        }
    }

    if (m_inputManager->IsKeyPressed(VK_BACK) && !m_tempStringInput.empty()) {
        m_tempStringInput.pop_back();
        m_needFullRedraw = true;
        return;
    }
}

void WorldEditor::ApplyEditedValue() {
    if (m_tempStringInput.empty()) return;

    try {
        switch (m_currentTab) {
        case EditorTab::WORLD:
        {
            std::vector<int> fieldMapping;
            for (int i = 0; i < 7; ++i) {
                if (i == 4 && !ShouldShowSeedField()) {
                    continue;
                }
                fieldMapping.push_back(i);
            }

            if (m_editingField < fieldMapping.size()) {
                int actualField = fieldMapping[m_editingField];
                switch (actualField) {
                case 0:
                    m_config.worldName = m_tempStringInput;
                    break;
                case 1:
                {
                    int newWidth = std::stoi(m_tempStringInput);
                    m_config.width = std::clamp(newWidth, MIN_WORLD_WIDTH, MAX_WORLD_WIDTH);
                    Logger::Log("World width set to: " + std::to_string(m_config.width));
                }
                break;
                case 2:
                {
                    int newHeight = std::stoi(m_tempStringInput);
                    m_config.height = std::clamp(newHeight, MIN_WORLD_HEIGHT, MAX_WORLD_HEIGHT);
                    Logger::Log("World height set to: " + std::to_string(m_config.height));
                }
                break;
                case 4: m_config.seed = std::stoi(m_tempStringInput); break;
                case 5: m_config.noiseFrequency = std::clamp(std::stof(m_tempStringInput), 0.1f, 1.0f); break;
                case 6: m_config.neighborRadius = max(1, std::stoi(m_tempStringInput)); break;
                }
            }
            break;
        }

        case EditorTab::PLAYER:
            switch (m_editingField) {
            case 0:
            {
                int newX = std::stoi(m_tempStringInput);
                m_config.playerStartX = std::clamp(newX, 1, m_config.width - 2);
            }
            break;
            case 1:
            {
                int newY = std::stoi(m_tempStringInput);
                m_config.playerStartY = std::clamp(newY, 1, m_config.height - 2);
            }
            break;
            case 2: m_config.playerMaxHP = max(10, std::stoi(m_tempStringInput)); break;
            case 3: m_config.playerMaxHunger = max(10, std::stoi(m_tempStringInput)); break;
            }
            break;
        }
    }
    catch (const std::exception& e) {
        Logger::Log("ERROR: Invalid input: " + m_tempStringInput);
    }
}
int WorldEditor::GetMaxFields() {
    switch (m_currentTab) {
    case EditorTab::WORLD:
        return GetVisibleWorldFieldsCount();
    case EditorTab::PLAYER: return 6;
    case EditorTab::TILES: return static_cast<int>(m_config.tileProbabilities.size());
    case EditorTab::CELLULAR_AUTOMATON: return 3;
    case EditorTab::FOOD: return 0;
    case EditorTab::ENEMIES: return 2;
    case EditorTab::WIN: return 0;
    case EditorTab::LOSE: return 0;
    default: return 0;
    }
}

void WorldEditor::SelectNextTab() {
    int current = static_cast<int>(m_currentTab);
    current = (current + 1) % 8;
    m_currentTab = static_cast<EditorTab>(current);
    m_selectedField = 0;
    m_selectedButton = 0;
    m_needFullRedraw = true;
}

void WorldEditor::ChangeFieldValue(int delta) {
    if (m_selectedButton != 0) return;

    switch (m_currentTab) {
    case EditorTab::WORLD:
        if (m_selectedField == 3) {
            m_config.randomGeneration = !m_config.randomGeneration;
        }
        break;
    case EditorTab::PLAYER:
        if (m_selectedField == 4) {
            m_config.enableHP = !m_config.enableHP;
        }
        else if (m_selectedField == 5) {
            m_config.enableHunger = !m_config.enableHunger;
        }
        break;
    case EditorTab::ENEMIES:
        if (m_selectedField == 0) {
            m_config.enableEnemies = !m_config.enableEnemies;
        }
        break;
    }
}

void WorldEditor::ConfirmSelection() {
    if (m_selectedButton == 0) {
        StartEditing();
    }
    else if (m_selectedButton == 1) {
        CreateNewWorld();
        m_shouldCreate = true;
    }
    else if (m_selectedButton == 2) {
        m_shouldReturn = true;
    }
}

void WorldEditor::CreateNewWorld() {
    Logger::Log("Creating new world in slot " + std::to_string(m_slot));
    Logger::Log("World name: " + m_config.worldName);
    Logger::Log("Size: " + std::to_string(m_config.width) + "x" + std::to_string(m_config.height));

    if (!m_saveSystem) {
        m_saveSystem = std::make_unique<SaveSystem>();
    }

    if (m_saveSystem->CreateNewSave(m_gameMode, m_slot, m_config.worldName, m_config)) {
        Logger::Log("Successfully created world: " + m_config.worldName);
        m_shouldCreate = true;

        SaveWorldConfiguration();
    }
    else {
        Logger::Log("ERROR: Failed to create world: " + m_config.worldName);
    }
}

void WorldEditor::SaveWorldConfiguration() {
    Logger::Log("World configuration saved successfully for: " + m_config.worldName);
}

void WorldEditor::ClearLine(int line) {
    rlutil::locate(0, line);
    for (int i = 0; i < 80; i++) {
        std::cout << ' ';
    }
}

bool WorldEditor::ShouldShowSeedField() const {
    return !m_config.randomGeneration;
}

int WorldEditor::GetVisibleWorldFieldsCount() const {
    int count = 7;
    if (!ShouldShowSeedField()) {
        count--;
    }
    return count;
}

void WorldEditor::HandleNumericInput() {
    for (char c = '0'; c <= '9'; c++) {
        if (m_inputManager->IsKeyPressed(c)) {
            m_tempStringInput += c;
            m_needFullRedraw = true;
            return;
        }
    }

    if (m_inputManager->IsKeyPressed(VK_BACK) && !m_tempStringInput.empty()) {
        m_tempStringInput.pop_back();
        m_needFullRedraw = true;
        return;
    }
}

void WorldEditor::HandleBooleanInput() {
    if (m_inputManager->IsKeyPressed('A') || m_inputManager->IsKeyPressed('D') ||
        m_inputManager->IsKeyPressed(VK_LEFT) || m_inputManager->IsKeyPressed(VK_RIGHT)) {
        m_config.randomGeneration = !m_config.randomGeneration;
        m_needFullRedraw = true;
        return;
    }
}

void WorldEditor::HandleFrequencyInput() {
    if (m_inputManager->IsKeyPressed(VK_LEFT) || m_inputManager->IsKeyPressed('A')) {
        m_config.noiseFrequency = max(0.1f, m_config.noiseFrequency - 0.1f);
        m_needFullRedraw = true;
        return;
    }
    else if (m_inputManager->IsKeyPressed(VK_RIGHT) || m_inputManager->IsKeyPressed('D')) {
        m_config.noiseFrequency = min(1.0f, m_config.noiseFrequency + 0.1f);
        m_needFullRedraw = true;
        return;
    }
}