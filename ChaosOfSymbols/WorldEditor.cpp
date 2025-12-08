#include "WorldEditor.h"
#include <windows.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>
#include "TemplateSystem.h"
#include <sstream>
#include "Logger.h"
#include "HelpPanel.h"
#include "HelpSystem.h"

namespace rlutil {
    void setColor(int color);
    void cls();
    void locate(int x, int y);
    void hideCursor();
}

WorldEditor::WorldEditor(EditorMode editorMode, int slot, GameMode gameMode)
    : m_editorMode(editorMode)
    , m_gameMode(gameMode)
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
    , m_tilesState(TilesState::MAIN_LIST)
    , m_prevTilesState(TilesState::MAIN_LIST)
    , m_selectedTileIndex(0)
    , m_editingTileField(false)
    , m_editingTileFieldIndex(0)
    , m_tileActionIndex(0)
    , m_editedTileName("")
    , m_editedTileSymbol(' ')
    , m_editedTileColor(7)
    , m_editedTileLowlandProb(0)
    , m_editedTilePlainsProb(0)
    , m_editedTileMountainProb(0)
    , m_newTileLowlandProb(0)
    , m_newTilePlainsProb(0)
    , m_newTileMountainProb(0)
{
    HelpPanel::Initialize();
    RegisterHelpSystemEntries();

    if (m_editorMode == EditorMode::CREATE_WORLD) {
        m_tilesConfigPath = "saves/proceduralGeneration/slot" + std::to_string(slot) + "/tiles.json";
    }
    else {
        m_tilesConfigPath = "templates\template_tiles_" + std::to_string(slot) + ".json";
    }

    CreateDirectoryForSlot(slot);

    m_inputManager = std::make_unique<InputManager>();

    m_tileManager = std::make_unique<TileTypeManager>(m_tilesConfigPath);

    std::ifstream file(m_tilesConfigPath);
    if (file.good()) {
        file.close();
        if (m_tileManager->LoadFromFile()) {
            Logger::Log("Loaded existing tiles from: " + m_tilesConfigPath);
        }
        else {
            Logger::Log("WARNING: Failed to load tiles from: " + m_tilesConfigPath);
            CreateDefaultTiles();
        }
    }
    else {
        Logger::Log("Tile config not found, creating default tiles");
        CreateDefaultTiles();
    }

    LoadAvailableTiles();

    m_newTileName = "new_tile";
    m_newTileSymbol = 'A';
    m_newTileColor = 7;
    m_newTileLowlandProb = 10;
    m_newTilePlainsProb = 10;
    m_newTileMountainProb = 10;

    Logger::Log("WorldEditor created for slot " + std::to_string(slot) +
        " with mode: " + (m_editorMode == EditorMode::CREATE_WORLD ? "CREATE_WORLD" : "CREATE_TEMPLATE") +
        " and " + std::to_string(m_availableTileIds.size()) + " tiles");
}

void WorldEditor::CreateDefaultTiles() {
    if (!m_tileManager) return;

    m_tileManager->RegisterTileType(TileType(0, "air", ' ', 0, true, false, 0, 0, 0, 0));
    m_tileManager->RegisterTileType(TileType(1, "grass", '.', 10, true, false, 0, 10, 80, 10));
    m_tileManager->RegisterTileType(TileType(2, "stone_wall", '#', 8, false, true, 0, 0, 0, 0));
    m_tileManager->RegisterTileType(TileType(3, "water", '~', 9, false, false, 0, 80, 20, 0));
    m_tileManager->RegisterTileType(TileType(4, "mountain", '^', 7, false, false, 0, 0, 10, 80));

    m_tileManager->SaveToFile();

    Logger::Log("Created default tiles for slot " + std::to_string(m_slot));
}

void WorldEditor::CreateDirectoryForSlot(int slot) {
    if (m_editorMode == EditorMode::CREATE_WORLD) {
        std::string dirPath = "saves/proceduralGeneration/slot" + std::to_string(slot);
#ifdef _WIN32
        CreateDirectoryA(dirPath.c_str(), NULL);
#else
        mkdir(dirPath.c_str(), 0777);
#endif
    }
    else {
        std::string tempDir = "templates";
#ifdef _WIN32
        CreateDirectoryA(tempDir.c_str(), NULL);
#else
        mkdir(tempDir.c_str(), 0777);
#endif
    }
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
    UpdateHelpForCurrentSelection();
    if (m_currentTab != m_prevTab) {
        rlutil::cls();
        m_needFullRedraw = true;
        m_prevTab = m_currentTab;
        m_prevFieldCount = 0;
    }


    if (m_needFullRedraw || NeedsRedraw()) {
        RenderOnlyChanges();
    }

    int screenHeight = 25;
    int screenWidth = 80;
    HelpPanel::Render(screenWidth, screenHeight);

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
                case 3: RenderMenuItem(line + visibleFieldIndex, "Random Generation: " + std::string(m_config.GetRandomGeneration() ? "Yes" : "No"), false); break;
                case 4: RenderEditField(line + visibleFieldIndex, "Seed: ", m_tempStringInput, true); break;
                case 5: RenderMenuItem(line + visibleFieldIndex, "Noise Frequency: " + std::to_string(m_config.GetNoiseFrequency()), false); break;
                case 6: RenderEditField(line + visibleFieldIndex, "Neighbor Radius: ", m_tempStringInput, true); break;
                }
            }
            else {
                switch (i) {
                case 0: RenderMenuItem(line + visibleFieldIndex, "World Name: " + m_config.GetWorldName(), false); break;
                case 1: RenderMenuItem(line + visibleFieldIndex, "Width: " + std::to_string(m_config.GetWidth()), false); break;
                case 2: RenderMenuItem(line + visibleFieldIndex, "Height: " + std::to_string(m_config.GetHeight()), false); break;
                case 3: RenderMenuItem(line + visibleFieldIndex, "Random Generation: " + std::string(m_config.GetRandomGeneration() ? "Yes" : "No"), false); break;
                case 4: RenderMenuItem(line + visibleFieldIndex, "Seed: " + std::to_string(m_config.GetSeed()), false); break;
                case 5: RenderMenuItem(line + visibleFieldIndex, "Noise Frequency: " + std::to_string(m_config.GetNoiseFrequency()), false); break;
                case 6: RenderMenuItem(line + visibleFieldIndex, "Neighbor Radius: " + std::to_string(m_config.GetNeighborRadius()), false); break;
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
        fields.push_back("World Name: " + m_config.GetWorldName());
        fields.push_back("Width: " + std::to_string(m_config.GetWidth()));
        fields.push_back("Height: " + std::to_string(m_config.GetHeight()));
        fields.push_back("Random Generation: " + std::string(m_config.GetRandomGeneration() ? "Yes" : "No"));

        if (ShouldShowSeedField()) {
            fields.push_back("Seed: " + std::to_string(m_config.GetSeed()));
        }

        fields.push_back("Noise Frequency: " + std::to_string(m_config.GetNoiseFrequency()));
        fields.push_back("Neighbor Radius: " + std::to_string(m_config.GetNeighborRadius()));

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
    if (m_currentTab == EditorTab::TILES &&
        (m_tilesState == TilesState::EDITING_TILE ||
            m_tilesState == TilesState::ADDING_TILE ||
            m_tilesState == TilesState::TILE_ACTIONS)) {
        return;
    }

    int line = 15;

    if (m_editorMode == EditorMode::CREATE_WORLD) {
        bool createSelected = (m_selectedButton == 1);
        bool backSelected = (m_selectedButton == 2);
        RenderMenuItem(line, "CREATE", createSelected);
        RenderMenuItem(line + 1, "BACK", backSelected);
    }
    else {
        bool createSelected = (m_selectedButton == 1);
        bool backSelected = (m_selectedButton == 2);
        RenderMenuItem(line, "SAVE TEMPLATE", createSelected);
        RenderMenuItem(line + 1, "BACK", backSelected);
    }
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
                case 4: RenderMenuItem(line + i, "Enable HP: " + std::string(m_config.GetEnableHP() ? "Yes" : "No"), false); break;
                case 5: RenderMenuItem(line + i, "Enable Hunger: " + std::string(m_config.GetEnableHunger() ? "Yes" : "No"), false); break;
                }
            }
            else {
                switch (i) {
                case 0: RenderMenuItem(line + i, "Start X: " + std::to_string(m_config.GetPlayerStartX()), false); break;
                case 1: RenderMenuItem(line + i, "Start Y: " + std::to_string(m_config.GetPlayerStartY()), false); break;
                case 2: RenderMenuItem(line + i, "Max HP: " + std::to_string(m_config.GetPlayerMaxHP()), false); break;
                case 3: RenderMenuItem(line + i, "Max Hunger: " + std::to_string(m_config.GetPlayerMaxHunger()), false); break;
                case 4: RenderMenuItem(line + i, "Enable HP: " + std::string(m_config.GetEnableHP() ? "Yes" : "No"), false); break;
                case 5: RenderMenuItem(line + i, "Enable Hunger: " + std::string(m_config.GetEnableHunger() ? "Yes" : "No"), false); break;
                }
            }
        }
    }
    else {
        std::vector<std::string> fields = {
            "Start X: " + std::to_string(m_config.GetPlayerStartX()),
            "Start Y: " + std::to_string(m_config.GetPlayerStartY()),
            "Max HP: " + std::to_string(m_config.GetPlayerMaxHP()),
            "Max Hunger: " + std::to_string(m_config.GetPlayerMaxHunger()),
            "Enable HP: " + std::string(m_config.GetEnableHP() ? "Yes" : "No"),
            "Enable Hunger: " + std::string(m_config.GetEnableHunger() ? "Yes" : "No")
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

    if (m_needFullRedraw) {
        for (int i = 5; i < 22; i++) {
            ClearLine(i);
        }

        rlutil::locate(2, 5);
        SetConsoleTextAttribute(hConsole, 14);
        std::cout << "Tile Configuration";
        SetConsoleTextAttribute(hConsole, 7);

        rlutil::locate(2, 6);
        std::cout << "------------------------------------------------------------";

        rlutil::locate(0, 8);
    }

    if (m_needFullRedraw || m_tilesState != m_prevTilesState) {
        for (int i = 8; i < 17; i++) {
            ClearLine(i);
        }
        m_prevTilesState = m_tilesState;

        rlutil::locate(0, 8);
    }

    switch (m_tilesState) {
    case TilesState::MAIN_LIST:
        RenderTileList(8);
        RenderBottomButtons();
        break;
    case TilesState::TILE_ACTIONS:
        RenderTileActions(8);
        break;
    case TilesState::EDITING_TILE:
        RenderTileEditing(8, false);
        break;
    case TilesState::ADDING_TILE:
        RenderTileEditing(8, true);
        break;
    }
}

void WorldEditor::RenderTileList(int startLine) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    int line = startLine;

    rlutil::locate(2, line);
    std::cout << "  Available Tiles:";
    line++;

    if (m_availableTileIds.empty()) {
        rlutil::locate(4, line);
        std::cout << "  No tiles available. Add your first tile to get started.";
        line += 2;
    }
    else {
        for (size_t i = 0; i < m_availableTileIds.size(); ++i) {
            int tileId = m_availableTileIds[i];
            TileType* tile = m_tileManager->GetTileType(tileId);

            if (!tile) continue;

            bool isSelected = (i == m_selectedField && m_selectedButton == 0);

            rlutil::locate(4, line);

            if (isSelected) {
                SetConsoleTextAttribute(hConsole, 10);
                std::cout << "> ";
            }
            else {
                SetConsoleTextAttribute(hConsole, 7);
                std::cout << "  ";
            }

            SetConsoleTextAttribute(hConsole, tile->GetColor());
            std::cout << tile->GetCharacter();

            SetConsoleTextAttribute(hConsole, isSelected ? 10 : 7);
            std::cout << " - " << tile->GetName();

            SetConsoleTextAttribute(hConsole, 7);
            line++;
        }
    }

    bool addSelected = (m_selectedField == GetMaxFields() - 1 && m_selectedButton == 0);
    rlutil::locate(4, line);

    if (addSelected) {
        SetConsoleTextAttribute(hConsole, 10);
        std::cout << "> ";
    }
    else {
        SetConsoleTextAttribute(hConsole, 7);
        std::cout << "  ";
    }
    std::cout << "+ Add New Tile";
    SetConsoleTextAttribute(hConsole, 7);
}

void WorldEditor::RenderTileActions(int startLine) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    int line = startLine;

    if (m_selectedTileIndex < static_cast<int>(m_availableTileIds.size())) {
        int tileId = m_availableTileIds[m_selectedTileIndex];
        TileType* tile = m_tileManager->GetTileType(tileId);

        if (tile) {
            rlutil::locate(4, line);
            SetConsoleTextAttribute(hConsole, tile->GetColor());
            std::cout << tile->GetCharacter();
            SetConsoleTextAttribute(hConsole, 7);
            std::cout << " - " << tile->GetName();
            line += 2;
        }
    }

    bool editSelected = (m_tileActionIndex == 0);
    RenderMenuItem(line, "- Edit", editSelected);
    line++;

    bool deleteSelected = (m_tileActionIndex == 1);
    RenderMenuItem(line, "- Delete", deleteSelected);
    line++;

    bool backSelected = (m_tileActionIndex == 2);
    RenderMenuItem(line, "- Back", backSelected);
}

void WorldEditor::RenderTileEditing(int startLine, bool isNewTile) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    int line = startLine;

    rlutil::locate(4, line);
    SetConsoleTextAttribute(hConsole, 14);
    if (isNewTile) {
        std::cout << "Add New Tile";
    }
    else {
        std::cout << "Edit Tile";
    }
    SetConsoleTextAttribute(hConsole, 7);
    line += 2;

    std::vector<std::string> fieldLabels = {
        "Symbol: ",
        "Color: ",
        "Name: ",
        "Lowland Probability: ",
        "Plains Probability: ",
        "Mountain Probability: "
    };

    for (int i = 0; i < fieldLabels.size(); ++i) {
        bool isSelected = (i == m_selectedField);
        bool isEditing = (m_isEditingText && m_editingTileFieldIndex == i);

        rlutil::locate(6, line + i);

        if (isSelected) {
            SetConsoleTextAttribute(hConsole, 10);
            std::cout << "> ";
        }
        else {
            SetConsoleTextAttribute(hConsole, 7);
            std::cout << "  ";
        }

        std::cout << fieldLabels[i];

        if (isEditing) {
            SetConsoleTextAttribute(hConsole, 11);
            std::cout << m_tempTileStringInput << "_";
        }
        else {
            SetConsoleTextAttribute(hConsole, 7);

            if (isNewTile) {
                switch (i) {
                case 0:
                    std::cout << "'";
                    SetConsoleTextAttribute(hConsole, m_newTileColor);
                    std::cout << m_newTileSymbol;
                    SetConsoleTextAttribute(hConsole, 7);
                    std::cout << "'";
                    break;
                case 1: std::cout << m_newTileColor; break;
                case 2: std::cout << m_newTileName; break;
                case 3: std::cout << m_newTileLowlandProb; break;
                case 4: std::cout << m_newTilePlainsProb; break;
                case 5: std::cout << m_newTileMountainProb; break;
                }
            }
            else {
                switch (i) {
                case 0:
                    std::cout << "'";
                    SetConsoleTextAttribute(hConsole, m_editedTileColor);
                    std::cout << m_editedTileSymbol;
                    SetConsoleTextAttribute(hConsole, 7);
                    std::cout << "'";
                    break;
                case 1: std::cout << m_editedTileColor; break;
                case 2: std::cout << m_editedTileName; break;
                case 3: std::cout << m_editedTileLowlandProb; break;
                case 4: std::cout << m_editedTilePlainsProb; break;
                case 5: std::cout << m_editedTileMountainProb; break;
                }
            }
        }

        SetConsoleTextAttribute(hConsole, 7);
    }

    int buttonsStart = line + fieldLabels.size() + 1;
    ClearLine(buttonsStart);
    ClearLine(buttonsStart + 1);

    bool saveSelected = (m_selectedField == static_cast<int>(fieldLabels.size()));
    RenderMenuItem(buttonsStart, "Save", saveSelected);

    bool cancelSelected = (m_selectedField == static_cast<int>(fieldLabels.size()) + 1);
    RenderMenuItem(buttonsStart + 1, "Cancel", cancelSelected);
}


void WorldEditor::RenderTileDetails() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    int detailLine = 8;
    int detailColumn = 40;

    int tileId = m_availableTileIds[m_selectedField];
    TileType* tile = m_tileManager->GetTileType(tileId);

    if (!tile) return;

    rlutil::locate(detailColumn, detailLine);
    SetConsoleTextAttribute(hConsole, 14);
    std::cout << "Tile Properties:";

    rlutil::locate(detailColumn, detailLine + 1);
    SetConsoleTextAttribute(hConsole, 7);
    std::cout << "-------------------------";

    rlutil::locate(detailColumn, detailLine + 3);
    std::cout << "ID: " << tile->GetId();

    rlutil::locate(detailColumn, detailLine + 4);
    std::cout << "Name: " << tile->GetName();

    rlutil::locate(detailColumn, detailLine + 5);
    std::cout << "Symbol: '";
    SetConsoleTextAttribute(hConsole, tile->GetColor());
    std::cout << tile->GetCharacter();
    SetConsoleTextAttribute(hConsole, 7);
    std::cout << "'";

    rlutil::locate(detailColumn, detailLine + 6);
    std::cout << "Color: " << tile->GetColor();

    rlutil::locate(detailColumn, detailLine + 7);
    std::cout << "Passable: " << (tile->IsPassable() ? "Yes" : "No");

    rlutil::locate(detailColumn, detailLine + 8);
    std::cout << "Destructible: " << (tile->IsDestructible() ? "Yes" : "No");

    rlutil::locate(detailColumn, detailLine + 9);
    std::cout << "Damage: " << tile->GetDamage();

    rlutil::locate(detailColumn, detailLine + 11);
    SetConsoleTextAttribute(hConsole, 8);
    std::cout << "A/D - Change color";

    rlutil::locate(detailColumn, detailLine + 12);
    std::cout << "ENTER - Edit properties";

    SetConsoleTextAttribute(hConsole, 7);
}

void WorldEditor::HandleTileInput() {
    if (m_tilesState == TilesState::EDITING_TILE || m_tilesState == TilesState::ADDING_TILE) {
        if (m_isEditingText && m_editingTileField) {
            HandleTileEditInput();
            return;
        }
        HandleTileEditNavigation();
        return;
    }

    if (m_tilesState == TilesState::TILE_ACTIONS) {
        HandleTileActionsNavigation();
        return;
    }

    HandleStandardInput();
}

void WorldEditor::HandleTileActionsNavigation() {
    if (m_inputManager->IsMenuUp()) {
        m_tileActionIndex = (m_tileActionIndex - 1 + 3) % 3;
    }
    else if (m_inputManager->IsMenuDown()) {
        m_tileActionIndex = (m_tileActionIndex + 1) % 3;
    }
    else if (m_inputManager->IsMenuSelect()) {
        switch (m_tileActionIndex) {
        case 0: // Edit
            StartEditingTile();
            break;
        case 1: // Delete
            DeleteSelectedTile();
            m_tilesState = TilesState::MAIN_LIST;
            m_selectedField = min(m_selectedField, static_cast<int>(m_availableTileIds.size()) - 1);
            if (m_selectedField < 0) m_selectedField = 0;
            m_needFullRedraw = true;
            break;
        case 2: // Back
            m_tilesState = TilesState::MAIN_LIST;
            m_needFullRedraw = true;
            break;
        }
    }
    else if (m_inputManager->IsMenuBack()) {
        m_tilesState = TilesState::MAIN_LIST;
        m_needFullRedraw = true;
    }
}

void WorldEditor::StartEditingTile() {
    if (m_availableTileIds.empty() || m_selectedTileIndex >= static_cast<int>(m_availableTileIds.size())) return;

    int tileId = m_availableTileIds[m_selectedTileIndex];
    TileType* tile = m_tileManager->GetTileType(tileId);

    if (!tile) return;

    m_editedTileName = tile->GetName();
    m_editedTileSymbol = tile->GetCharacter();
    m_editedTileColor = tile->GetColor();
    m_editedTileLowlandProb = tile->GetLowlandProbability();
    m_editedTilePlainsProb = tile->GetPlainsProbability();
    m_editedTileMountainProb = tile->GetMountainProbability();

    m_tilesState = TilesState::EDITING_TILE;
    m_selectedField = 0;
    m_editingTileField = false;
    m_needFullRedraw = true;
}


void WorldEditor::HandleTileEditNavigation() {
    int fieldCount = 6;
    int buttonCount = 2;
    int totalCount = fieldCount + buttonCount;

    if (m_inputManager->IsMenuUp()) {
        m_selectedField = (m_selectedField - 1 + totalCount) % totalCount;
    }
    else if (m_inputManager->IsMenuDown()) {
        m_selectedField = (m_selectedField + 1) % totalCount;
    }
    else if (m_inputManager->IsMenuSelect()) {
        if (m_selectedField < fieldCount) {
            StartEditingTileField();
        }
        else if (m_selectedField == fieldCount) {
            if (m_tilesState == TilesState::ADDING_TILE) {
                AddNewTile();
                m_tilesState = TilesState::MAIN_LIST;
                m_selectedField = 0;
                m_selectedButton = 0;
            }
            else {
                ApplyTileEdit();
                m_tilesState = TilesState::MAIN_LIST;
                m_selectedField = m_selectedTileIndex;
                m_selectedButton = 0;
            }
            m_needFullRedraw = true;
        }
        else if (m_selectedField == fieldCount + 1) {
            m_tilesState = TilesState::MAIN_LIST;
            m_selectedField = (m_tilesState == TilesState::MAIN_LIST && m_availableTileIds.empty()) ? 0 :
                (m_selectedTileIndex < static_cast<int>(m_availableTileIds.size()) ? m_selectedTileIndex : 0);
            m_selectedButton = 0;
            m_needFullRedraw = true;
        }
    }
    else if (m_inputManager->IsMenuBack()) {
        m_tilesState = TilesState::MAIN_LIST;
        m_selectedField = (m_tilesState == TilesState::MAIN_LIST && m_availableTileIds.empty()) ? 0 :
            (m_selectedTileIndex < static_cast<int>(m_availableTileIds.size()) ? m_selectedTileIndex : 0);
        m_selectedButton = 0;
        m_needFullRedraw = true;
    }
}

void WorldEditor::StartAddingTile() {
    m_newTileName = "new_tile";
    m_newTileSymbol = 'A';
    m_newTileColor = 7;
    m_newTileLowlandProb = 10;
    m_newTilePlainsProb = 10;
    m_newTileMountainProb = 10;

    m_tilesState = TilesState::ADDING_TILE;
    m_selectedField = 0;
    m_editingTileField = false;
    m_needFullRedraw = true;
}

void WorldEditor::StartEditingTileField() {
    m_isEditingText = true;
    m_editingTileField = true;
    m_editingTileFieldIndex = m_selectedField;

    m_tempTileStringInput = "";

    if (m_tilesState == TilesState::EDITING_TILE && m_selectedTileIndex < static_cast<int>(m_availableTileIds.size())) {
        int tileId = m_availableTileIds[m_selectedTileIndex];
        TileType* tile = m_tileManager->GetTileType(tileId);

        if (tile) {
            switch (m_editingTileFieldIndex) {
            case 0: // Symbol
                m_tempTileStringInput = std::string(1, tile->GetCharacter());
                break;
            case 1: // Color
                m_tempTileStringInput = std::to_string(tile->GetColor());
                break;
            case 2: // Name
                m_tempTileStringInput = tile->GetName();
                break;
            case 3: // Lowland Probability
                m_tempTileStringInput = std::to_string(tile->GetLowlandProbability());
                break;
            case 4: // Plains Probability
                m_tempTileStringInput = std::to_string(tile->GetPlainsProbability());
                break;
            case 5: // Mountain Probability
                m_tempTileStringInput = std::to_string(tile->GetMountainProbability());
                break;
            }
        }
    }
    else if (m_tilesState == TilesState::ADDING_TILE) {
        switch (m_editingTileFieldIndex) {
        case 0: // Symbol
            m_tempTileStringInput = std::string(1, m_newTileSymbol);
            break;
        case 1: // Color
            m_tempTileStringInput = std::to_string(m_newTileColor);
            break;
        case 2: // Name
            m_tempTileStringInput = m_newTileName;
            break;
        case 3: // Lowland Probability
            m_tempTileStringInput = std::to_string(m_newTileLowlandProb);
            break;
        case 4: // Plains Probability
            m_tempTileStringInput = std::to_string(m_newTilePlainsProb);
            break;
        case 5: // Mountain Probability
            m_tempTileStringInput = std::to_string(m_newTileMountainProb);
            break;
        }
    }

    m_needFullRedraw = true;
}

void WorldEditor::HandleTileEditInput() {
    if (m_inputManager->IsKeyPressed(VK_RETURN) || m_inputManager->IsKeyPressed(VK_SPACE)) {
        if (m_tilesState == TilesState::ADDING_TILE) {
            SaveNewTileField();
        }
        else {
            SaveEditedTileField();
        }
        m_isEditingText = false;
        m_editingTileField = false;
        m_needFullRedraw = true;
        return;
    }

    if (m_inputManager->IsKeyPressed(VK_ESCAPE)) {
        m_isEditingText = false;
        m_editingTileField = false;
        m_needFullRedraw = true;
        return;
    }

    if (m_inputManager->IsKeyPressed(VK_TAB)) {
        if (m_tilesState == TilesState::ADDING_TILE) {
            SaveNewTileField();
        }
        else {
            SaveEditedTileField();
        }

        m_editingTileFieldIndex = (m_editingTileFieldIndex + 1) % 6;
        m_tempTileStringInput = "";

        StartEditingTileField();
        m_needFullRedraw = true;
        return;
    }

    if (m_editingTileFieldIndex >= 3 && m_editingTileFieldIndex <= 5) {
        HandleProbabilityInput();
    }
    else {
        switch (m_editingTileFieldIndex) {
        case 0:
            HandleSymbolInput();
            break;
        case 1:
            HandleColorInput();
            break;
        case 2:
            HandleTileNameInput();
            break;
        }
    }
}

void WorldEditor::HandleProbabilityInput() {
    for (char c = '0'; c <= '9'; c++) {
        if (m_inputManager->IsKeyPressed(c)) {
            if (m_tempTileStringInput.empty() && c == '0') {
                m_tempTileStringInput += c;
            }
            else if (!m_tempTileStringInput.empty() && m_tempTileStringInput != "0") {
                std::string testValue = m_tempTileStringInput + c;
                try {
                    int testNum = std::stoi(testValue);
                    if (testNum <= 100) {
                        m_tempTileStringInput = testValue;
                    }
                }
                catch (...) {
                    m_tempTileStringInput = std::string(1, c);
                }
            }
            else {
                m_tempTileStringInput += c;
            }
            m_needFullRedraw = true;
            return;
        }
    }

    if (m_inputManager->IsKeyPressed(VK_BACK) && !m_tempTileStringInput.empty()) {
        m_tempTileStringInput.pop_back();
        m_needFullRedraw = true;
        return;
    }
}

void WorldEditor::HandleSymbolInput() {
    for (char c = 32; c <= 126; c++) {
        if (m_inputManager->IsKeyPressed(c)) {
            m_tempTileStringInput = std::string(1, c);
            m_needFullRedraw = true;
            return;
        }
    }

    if (m_inputManager->IsKeyPressed(VK_BACK) && !m_tempTileStringInput.empty()) {
        m_tempTileStringInput.clear();
        m_needFullRedraw = true;
        return;
    }
}

void WorldEditor::HandleColorInput() {
    if (m_inputManager->IsKeyPressed(VK_LEFT) || m_inputManager->IsKeyPressed('A')) {
        if (m_tempTileStringInput.empty()) {
            m_tempTileStringInput = "7";
        }
        else {
            try {
                int currentColor = std::stoi(m_tempTileStringInput);
                currentColor = max(0, currentColor - 1);
                m_tempTileStringInput = std::to_string(currentColor);
            }
            catch (...) {
                m_tempTileStringInput = "0";
            }
        }
        m_needFullRedraw = true;
        return;
    }

    if (m_inputManager->IsKeyPressed(VK_RIGHT) || m_inputManager->IsKeyPressed('D')) {
        if (m_tempTileStringInput.empty()) {
            m_tempTileStringInput = "7";
        }
        else {
            try {
                int currentColor = std::stoi(m_tempTileStringInput);
                currentColor = min(15, currentColor + 1);
                m_tempTileStringInput = std::to_string(currentColor);
            }
            catch (...) {
                m_tempTileStringInput = "15";
            }
        }
        m_needFullRedraw = true;
        return;
    }

    for (char c = '0'; c <= '9'; c++) {
        if (m_inputManager->IsKeyPressed(c)) {
            if (m_tempTileStringInput.empty() || m_tempTileStringInput == "0") {
                m_tempTileStringInput = std::string(1, c);
            }
            else {
                std::string newValue = m_tempTileStringInput + c;
                try {
                    int color = std::stoi(newValue);
                    if (color <= 15) {
                        m_tempTileStringInput = newValue;
                    }
                    else {
                        m_tempTileStringInput = std::string(1, c);
                    }
                }
                catch (...) {
                    m_tempTileStringInput = std::string(1, c);
                }
            }
            m_needFullRedraw = true;
            return;
        }
    }

    if (m_inputManager->IsKeyPressed(VK_BACK) && !m_tempTileStringInput.empty()) {
        m_tempTileStringInput.pop_back();
        m_needFullRedraw = true;
        return;
    }
}

void WorldEditor::HandleTileNameInput() {
    for (char c = '0'; c <= '9'; c++) {
        if (m_inputManager->IsKeyPressed(c)) {
            m_tempTileStringInput += c;
            m_needFullRedraw = true;
            return;
        }
    }

    for (char c = 'A'; c <= 'Z'; c++) {
        if (m_inputManager->IsKeyPressed(c)) {
            m_tempTileStringInput += c;
            m_needFullRedraw = true;
            return;
        }
    }

    for (char c = 'a'; c <= 'z'; c++) {
        if (m_inputManager->IsKeyPressed(c)) {
            m_tempTileStringInput += c;
            m_needFullRedraw = true;
            return;
        }
    }

    if (m_inputManager->IsKeyPressed('_')) {
        m_tempTileStringInput += '_';
        m_needFullRedraw = true;
        return;
    }

    if (m_inputManager->IsKeyPressed('-')) {
        m_tempTileStringInput += '-';
        m_needFullRedraw = true;
        return;
    }

    if (m_inputManager->IsKeyPressed(VK_BACK) && !m_tempTileStringInput.empty()) {
        m_tempTileStringInput.pop_back();
        m_needFullRedraw = true;
        return;
    }
}

void WorldEditor::SaveEditedTileField() {
    if (m_tempTileStringInput.empty()) return;

    if (m_tilesState == TilesState::EDITING_TILE) {
        try {
            switch (m_editingTileFieldIndex) {
            case 0:
                if (!m_tempTileStringInput.empty()) {
                    m_editedTileSymbol = m_tempTileStringInput[0];
                }
                break;
            case 1:
            {
                int newColor = std::stoi(m_tempTileStringInput);
                m_editedTileColor = std::clamp(newColor, 0, 15);
            }
            break;
            case 2:
                m_editedTileName = m_tempTileStringInput;
                break;
            case 3:
            {
                int prob = std::stoi(m_tempTileStringInput);
                m_editedTileLowlandProb = std::clamp(prob, 0, 100);
            }
            break;
            case 4:
            {
                int prob = std::stoi(m_tempTileStringInput);
                m_editedTilePlainsProb = std::clamp(prob, 0, 100);
            }
            break;
            case 5:
            {
                int prob = std::stoi(m_tempTileStringInput);
                m_editedTileMountainProb = std::clamp(prob, 0, 100);
            }
            break;
            }
        }
        catch (const std::exception& e) {
            Logger::Log("ERROR: Invalid tile input: " + m_tempTileStringInput);
        }
    }
}

void WorldEditor::SaveNewTileField() {
    if (m_tempTileStringInput.empty()) return;

    try {
        switch (m_editingTileFieldIndex) {
        case 0:
            if (!m_tempTileStringInput.empty()) {
                m_newTileSymbol = m_tempTileStringInput[0];
            }
            break;
        case 1:
        {
            int newColor = std::stoi(m_tempTileStringInput);
            m_newTileColor = std::clamp(newColor, 0, 15);
        }
        break;
        case 2:
            m_newTileName = m_tempTileStringInput;
            break;
        case 3:
        {
            int prob = std::stoi(m_tempTileStringInput);
            m_newTileLowlandProb = std::clamp(prob, 0, 100);
        }
        break;
        case 4:
        {
            int prob = std::stoi(m_tempTileStringInput);
            m_newTilePlainsProb = std::clamp(prob, 0, 100);
        }
        break;
        case 5:
        {
            int prob = std::stoi(m_tempTileStringInput);
            m_newTileMountainProb = std::clamp(prob, 0, 100);
        }
        break;
        }
    }
    catch (const std::exception& e) {
        Logger::Log("ERROR: Invalid tile input: " + m_tempTileStringInput);
    }
}

void WorldEditor::ApplyTileEdit() {
    if (m_tilesState == TilesState::EDITING_TILE) {
        int tileId = m_availableTileIds[m_selectedTileIndex];
        TileType* tile = m_tileManager->GetTileType(tileId);

        if (!tile) return;

        try {
            TileType newTile(tileId, m_editedTileName, m_editedTileSymbol, m_editedTileColor,
                true, false, 0,
                m_editedTileLowlandProb, m_editedTilePlainsProb, m_editedTileMountainProb);

            m_tileManager->RegisterTileType(newTile);

            m_tileManager->SaveToFile();

            LoadAvailableTiles();

            Logger::Log("Tile updated: " + m_editedTileName + " (ID: " + std::to_string(tileId) + ")" +
                " Symbol: '" + std::string(1, m_editedTileSymbol) + "'" +
                " L:" + std::to_string(m_editedTileLowlandProb) +
                " P:" + std::to_string(m_editedTilePlainsProb) +
                " M:" + std::to_string(m_editedTileMountainProb));

        }
        catch (const std::exception& e) {
            Logger::Log("ERROR: Failed to update tile: " + std::string(e.what()));
        }
    }
}

void WorldEditor::ChangeTileColor(int delta) {
    int tileId = m_availableTileIds[m_selectedField];
    TileType* tile = m_tileManager->GetTileType(tileId);

    if (!tile) return;

    int newColor = tile->GetColor() + delta;
    newColor = std::clamp(newColor, 0, 15);

    TileType newTile = *tile;
    newTile.SetColor(newColor);
    m_tileManager->RegisterTileType(newTile);
    m_tileManager->SaveToFile();

    m_needFullRedraw = true;
}

void WorldEditor::AddNewTile() {
    int newId = 0;
    for (int id : m_availableTileIds) {
        if (id >= newId) newId = id + 1;
    }

    try {
        TileType newTile(newId, m_newTileName, m_newTileSymbol, m_newTileColor,
            true, false, 0,
            m_newTileLowlandProb, m_newTilePlainsProb, m_newTileMountainProb);

        m_tileManager->RegisterTileType(newTile);

        m_tileManager->SaveToFile();

        LoadAvailableTiles();

        Logger::Log("Added new tile: " + m_newTileName + " (ID: " + std::to_string(newId) +
            ") Symbol: '" + std::string(1, m_newTileSymbol) + "'" +
            " L:" + std::to_string(m_newTileLowlandProb) +
            " P:" + std::to_string(m_newTilePlainsProb) +
            " M:" + std::to_string(m_newTileMountainProb));

        m_newTileName = "new_tile_" + std::to_string(newId + 1);
        m_newTileSymbol = static_cast<char>('A' + ((newId + 1) % 26));
        m_newTileColor = 7;
        m_newTileLowlandProb = 10;
        m_newTilePlainsProb = 10;
        m_newTileMountainProb = 10;

        m_editedTileName = "";
        m_editedTileSymbol = ' ';
        m_editedTileColor = 7;
        m_editedTileLowlandProb = 0;
        m_editedTilePlainsProb = 0;
        m_editedTileMountainProb = 0;

    }
    catch (const std::exception& e) {
        Logger::Log("ERROR: Failed to add new tile: " + std::string(e.what()));
    }
}

void WorldEditor::DeleteSelectedTile() {
    if (m_availableTileIds.empty() || m_selectedTileIndex >= static_cast<int>(m_availableTileIds.size())) return;

    int tileId = m_availableTileIds[m_selectedTileIndex];

    if (tileId <= 2) {
        Logger::Log("Cannot delete basic tiles (ID 0-2)");
        return;
    }

    m_tileManager->SaveToFile();

    LoadAvailableTiles();

    if (m_selectedTileIndex >= static_cast<int>(m_availableTileIds.size())) {
        m_selectedTileIndex = max(0, static_cast<int>(m_availableTileIds.size()) - 1);
    }

    Logger::Log("Deleted tile with ID: " + std::to_string(tileId));
}

void WorldEditor::LoadAvailableTiles() {
    m_availableTileIds.clear();

    if (!m_tileManager) return;

    const auto& allTiles = m_tileManager->GetAllTiles();
    Logger::Log("Tile manager has " + std::to_string(allTiles.size()) + " tiles");

    for (const auto& pair : allTiles) {
        m_availableTileIds.push_back(pair.first);

        const TileType& tile = pair.second;
        Logger::Log("  Tile ID " + std::to_string(tile.GetId()) + ": '" +
            std::string(1, tile.GetCharacter()) + "' - " + tile.GetName() +
            " L:" + std::to_string(tile.GetLowlandProbability()) +
            " P:" + std::to_string(tile.GetPlainsProbability()) +
            " M:" + std::to_string(tile.GetMountainProbability()));
    }

    std::sort(m_availableTileIds.begin(), m_availableTileIds.end());

    Logger::Log("Loaded " + std::to_string(m_availableTileIds.size()) + " tiles into available list");
}

void WorldEditor::RenderCellularAutomatonTab() {
    int line = 6;

    std::vector<std::string> fields = {
        "Survival Rules: " + m_config.GetSurvivalRules(),
        "Birth Rules: " + m_config.GetBirthRules(),
        "Death Rules: " + m_config.GetDeathRules()
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
        "Enable Enemies: " + std::string(m_config.GetEnableEnemies() ? "Yes" : "No"),
        "Enemy Spawn Rate: " + std::to_string(m_config.GetEnemySpawnRate())
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

    for (int i = 4; i < 80; i++) {
        std::cout << ' ';
    }
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

void WorldEditor::ProcessInput() {
    if (!m_inputManager) return;

    if (m_inputManager->IsKeyPressed(VK_TAB)) {
        SelectNextTab();
        return;
    }

    if (m_isEditingText) {
        if (m_editingTileField) {
            HandleTileEditInput();
            return;
        }
        HandleTextInput();
        return;
    }

    if (m_inputManager->IsMenuBack()) {
        if (m_tilesState == TilesState::MAIN_LIST && m_selectedButton == 0) {
            m_shouldReturn = true;
        }
        else if (m_tilesState != TilesState::MAIN_LIST) {
            m_tilesState = TilesState::MAIN_LIST;
            m_selectedField = 0;
            m_selectedButton = 0;
            m_needFullRedraw = true;
        }
        else {
            m_shouldReturn = true;
        }
        return;
    }

    switch (m_currentTab) {
    case EditorTab::TILES:
        HandleTileInput();
        break;
    case EditorTab::WORLD:
    case EditorTab::PLAYER:
    case EditorTab::CELLULAR_AUTOMATON:
    case EditorTab::FOOD:
    case EditorTab::ENEMIES:
    case EditorTab::WIN:
    case EditorTab::LOSE:
        HandleStandardInput();
        break;
    }
}

void WorldEditor::HandleStandardInput() {
    if (m_inputManager->IsMenuUp()) {
        SelectPreviousOption();
    }
    else if (m_inputManager->IsMenuDown()) {
        SelectNextOption();
    }
    else if (m_inputManager->IsMenuSelect()) {
        ConfirmSelection();
    }
}

void WorldEditor::SelectNextOption() {
    if (m_selectedButton == 0) {
        if (m_selectedField < GetMaxFields() - 1) {
            m_selectedField++;
        }
        else {
            m_selectedButton = 1; // CREATE
            m_selectedField = GetMaxFields() - 1;
        }
    }
    else {
        if (m_selectedButton == 1) {
            m_selectedButton = 2; // BACK
        }
        else if (m_selectedButton == 2) {
            m_selectedButton = 0; // Возврат к списку
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
            m_selectedButton = 2; // BACK
            m_selectedField = 0;
        }
    }
    else {
        if (m_selectedButton == 2) {
            m_selectedButton = 1; // CREATE
        }
        else if (m_selectedButton == 1) {
            m_selectedButton = 0; // Возврат к списку
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
            case 0: m_tempStringInput = m_config.GetWorldName(); break;
            case 1: m_tempStringInput = std::to_string(m_config.GetWidth()); break;
            case 2: m_tempStringInput = std::to_string(m_config.GetHeight()); break;
            case 3:
                m_isEditingText = false;
                m_config.SetRandomGeneration(!m_config.GetRandomGeneration());
                m_needFullRedraw = true;
                return;
            case 4: m_tempStringInput = std::to_string(m_config.GetSeed()); break;
            case 5: m_tempStringInput = std::to_string(m_config.GetNoiseFrequency()); break;
            case 6: m_tempStringInput = std::to_string(m_config.GetNeighborRadius()); break;
            }
        }
        break;
    }

    case EditorTab::PLAYER:
        switch (m_editingField) {
        case 0: m_tempStringInput = std::to_string(m_config.GetPlayerStartX()); break;
        case 1: m_tempStringInput = std::to_string(m_config.GetPlayerStartY()); break;
        case 2: m_tempStringInput = std::to_string(m_config.GetPlayerMaxHP()); break;
        case 3: m_tempStringInput = std::to_string(m_config.GetPlayerMaxHunger()); break;
        case 4:
            m_isEditingText = false;
            m_config.SetEnableHP(!m_config.GetEnableHP());
            m_needFullRedraw = true;
            return;
        case 5:
            m_isEditingText = false;
            m_config.SetEnableHunger(!m_config.GetEnableHunger());
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
                    m_config.SetWorldName(m_tempStringInput);
                    break;
                case 1:
                {
                    int newWidth = std::stoi(m_tempStringInput);
                    int clampedWidth = std::clamp(newWidth, MIN_WORLD_WIDTH, MAX_WORLD_WIDTH);
                    m_config.SetWidth(clampedWidth);
                    Logger::Log("World width set to: " + std::to_string(m_config.GetWidth()));
                }
                break;
                case 2:
                {
                    int newHeight = std::stoi(m_tempStringInput);
                    int clampedHeight = std::clamp(newHeight, MIN_WORLD_HEIGHT, MAX_WORLD_HEIGHT);
                    m_config.SetHeight(clampedHeight);
                    Logger::Log("World height set to: " + std::to_string(m_config.GetHeight()));
                }
                break;
                case 4: m_config.SetSeed(std::stoi(m_tempStringInput)); break;
                case 5: m_config.SetNoiseFrequency(std::clamp<float>(std::stof(m_tempStringInput), 0.1f, 1.0f)); break;
                case 6: m_config.SetNeighborRadius(max(1, std::stoi(m_tempStringInput))); break;
                }
            }
            break;
        }

        case EditorTab::PLAYER:
            switch (m_editingField) {
            case 0:
            {
                int newX = std::stoi(m_tempStringInput);
                int clampedX = std::clamp(newX, 1, m_config.GetWidth() - 2);
                m_config.SetPlayerStartX(clampedX);
            }
            break;
            case 1:
            {
                int newY = std::stoi(m_tempStringInput);
                int clampedY = std::clamp(newY, 1, m_config.GetHeight() - 2);
                m_config.SetPlayerStartY(clampedY);
            }
            break;
            case 2: m_config.SetPlayerMaxHP(max(10, std::stoi(m_tempStringInput))); break;
            case 3: m_config.SetPlayerMaxHunger(max(10, std::stoi(m_tempStringInput))); break;
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
    case EditorTab::PLAYER:
        return 6;
    case EditorTab::TILES:
        if (m_tilesState == TilesState::MAIN_LIST) {
            if (m_availableTileIds.empty()) {
                return 1;
            }
            return static_cast<int>(m_availableTileIds.size()) + 1;
        }
        else if (m_tilesState == TilesState::TILE_ACTIONS) {
            return 3;
        }
        else if (m_tilesState == TilesState::EDITING_TILE || m_tilesState == TilesState::ADDING_TILE) {
            return 5;
        }
        return 0;
    case EditorTab::CELLULAR_AUTOMATON:
        return 3;
    case EditorTab::FOOD:
        return 0;
    case EditorTab::ENEMIES:
        return 2;
    case EditorTab::WIN:
        return 0;
    case EditorTab::LOSE:
        return 0;
    default:
        return 0;
    }
}

void WorldEditor::ClearDefaultTiles() {
    m_availableTileIds.clear();
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
            m_config.SetRandomGeneration(!m_config.GetRandomGeneration());
        }
        break;
    case EditorTab::PLAYER:
        if (m_selectedField == 4) {
            m_config.SetEnableHP(!m_config.GetEnableHP());
        }
        else if (m_selectedField == 5) {
            m_config.SetEnableHunger(!m_config.GetEnableHunger());
        }
        break;
    case EditorTab::ENEMIES:
        if (m_selectedField == 0) {
            m_config.SetEnableEnemies(!m_config.GetEnableEnemies());
        }
        break;
    }
}

void WorldEditor::ConfirmSelection() {
    if (m_currentTab == EditorTab::TILES && m_tilesState == TilesState::MAIN_LIST) {
        if (m_selectedButton == 0) {
            if (m_availableTileIds.empty()) {
                if (m_selectedField == 0) {
                    StartAddingTile();
                }
            }
            else {
                if (m_selectedField < static_cast<int>(m_availableTileIds.size())) {
                    m_selectedTileIndex = m_selectedField;
                    m_tileActionIndex = 0;
                    m_tilesState = TilesState::TILE_ACTIONS;
                    m_needFullRedraw = true;
                }
                else if (m_selectedField == static_cast<int>(m_availableTileIds.size())) {
                    StartAddingTile();
                }
            }
        }
        else if (m_selectedButton == 1) {
            CreateNewWorld();
            m_shouldCreate = true;
        }
        else if (m_selectedButton == 2) {
            m_shouldReturn = true;
        }
    }
    else if (m_currentTab == EditorTab::TILES && m_tilesState == TilesState::TILE_ACTIONS) {
        switch (m_tileActionIndex) {
        case 0: // Edit
            StartEditingTile();
            break;
        case 1: // Delete
            DeleteSelectedTile();
            m_tilesState = TilesState::MAIN_LIST;
            m_selectedField = min(m_selectedField, static_cast<int>(m_availableTileIds.size()) - 1);
            if (m_selectedField < 0) m_selectedField = 0;
            m_needFullRedraw = true;
            break;
        case 2: // Back
            m_tilesState = TilesState::MAIN_LIST;
            m_needFullRedraw = true;
            break;
        }
    }
    else {
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
}

void WorldEditor::CreateNewWorld() {
    Logger::Log("=== CREATE NEW WORLD STARTED ===");
    Logger::Log("World name: " + m_config.GetWorldName());
    Logger::Log("Size: " + std::to_string(m_config.GetWidth()) + "x" + std::to_string(m_config.GetHeight()));
    Logger::Log("Editor mode: " + std::to_string(static_cast<int>(m_editorMode)));
    Logger::Log("Slot: " + std::to_string(m_slot));

    if (m_editorMode == EditorMode::CREATE_WORLD) {
        if (!m_saveSystem) {
            m_saveSystem = std::make_unique<SaveSystem>();
        }

        if (m_saveSystem->CreateNewSave(m_gameMode, m_slot, m_config.GetWorldName(), m_config)) {
            Logger::Log("Successfully created world: " + m_config.GetWorldName());
            m_shouldCreate = true;
            SaveWorldConfiguration();
        }
        else {
            Logger::Log("ERROR: Failed to create world: " + m_config.GetWorldName());
        }
    }
    else {
        Logger::Log("=== CREATING TEMPLATE ===");
        Logger::Log("Using world name as template name: " + m_config.GetWorldName());

        SaveWorldConfiguration();

        if (CreateTemplate(m_config.GetWorldName())) {
            Logger::Log("=== TEMPLATE '" + m_config.GetWorldName() + "' CREATED SUCCESSFULLY ===");
            m_shouldCreate = true;
        }
        else {
            Logger::Log("=== TEMPLATE CREATION FAILED ===");
        }
    }
}

void WorldEditor::SaveWorldConfiguration() {
    Logger::Log("World configuration saved successfully for: " + m_config.GetWorldName());

    if (m_tileManager) {
        const auto& allTiles = m_tileManager->GetAllTiles();
        std::unordered_map<char, std::vector<float>> tileProbabilities;

        for (const auto& pair : allTiles) {
            const TileType& tile = pair.second;

            std::vector<float> probs = {
                static_cast<float>(tile.GetLowlandProbability()),
                static_cast<float>(tile.GetPlainsProbability()),
                static_cast<float>(tile.GetMountainProbability())
            };

            tileProbabilities[tile.GetCharacter()] = probs;

            Logger::Log("Saved tile '" + std::string(1, tile.GetCharacter()) +
                "' probabilities: L=" + std::to_string(probs[0]) +
                "%, P=" + std::to_string(probs[1]) +
                "%, M=" + std::to_string(probs[2]) + "%");
        }

        m_config.SetTileProbabilities(tileProbabilities);
    }
}

void WorldEditor::ClearLine(int line) {
    rlutil::locate(0, line);
    for (int i = 0; i < 80; i++) {
        std::cout << ' ';
    }
}

bool WorldEditor::ShouldShowSeedField() const {
    return !m_config.GetRandomGeneration();
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
        m_config.SetRandomGeneration(!m_config.GetRandomGeneration());
        m_needFullRedraw = true;
        return;
    }
}

void WorldEditor::HandleFrequencyInput() {
    if (m_inputManager->IsKeyPressed(VK_LEFT) || m_inputManager->IsKeyPressed('A')) {
        m_config.SetNoiseFrequency(max(0.1f, m_config.GetNoiseFrequency() - 0.1f));
        m_needFullRedraw = true;
        return;
    }
    else if (m_inputManager->IsKeyPressed(VK_RIGHT) || m_inputManager->IsKeyPressed('D')) {
        m_config.SetNoiseFrequency(min(1.0f, m_config.GetNoiseFrequency() + 0.1f));
        m_needFullRedraw = true;
        return;
    }
}

void WorldEditor::RenderEditField(int line, const std::string& label, const std::string& value, bool selected) {
    rlutil::locate(4, line);

    for (int i = 4; i < 80; i++) {
        std::cout << ' ';
    }
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

bool WorldEditor::CreateTemplate(const std::string& templateName) {
    Logger::Log("=== CREATE TEMPLATE FUNCTION STARTED ===");
    Logger::Log("Template name from World Name field: " + templateName);
    Logger::Log("Slot: " + std::to_string(m_slot));

    if (m_editorMode != EditorMode::CREATE_TEMPLATE) {
        Logger::Log("ERROR: Cannot create template in CREATE_WORLD mode");
        return false;
    }

    TemplateSystem templateSystem;
    if (!templateSystem.Initialize()) {
        Logger::Log("ERROR: Failed to initialize TemplateSystem");
        return false;
    }

    SaveWorldConfiguration();

    if (templateSystem.CreateTemplate(m_slot, templateName, m_config)) {
        Logger::Log("=== TEMPLATE '" + templateName + "' CREATED SUCCESSFULLY ===");

        std::string templateDir = "templates/template" + std::to_string(m_slot);

        SaveWorldConfig(templateDir);
        SavePlayerConfig(templateDir);
        SaveTilesConfig(templateDir);
        SaveCellularAutomatonConfig(templateDir);
        SaveFoodConfig(templateDir);
        SaveEnemiesConfig(templateDir);

        if (m_tileManager) {
            std::string tilesJsonPath = templateDir + "/tiles.json";
            m_tileManager->SaveToFile(tilesJsonPath);
        }

        m_shouldCreate = true;
        return true;
    }
    else {
        Logger::Log("=== TEMPLATE CREATION FAILED ===");
        return false;
    }
}

bool WorldEditor::SaveAllConfigurations(const std::string& directory) {
    Logger::Log("=== SAVING ALL CONFIGURATIONS TO: " + directory + " ===");

    if (!fs::exists(directory)) {
        if (!fs::create_directories(directory)) {
            Logger::Log("ERROR: Failed to create directory: " + directory);
            return false;
        }
        Logger::Log("Created directory: " + directory);
    }

    bool success = true;

    success = SaveWorldConfig(directory) && success;
    success = SavePlayerConfig(directory) && success;
    success = SaveTilesConfig(directory) && success;
    success = SaveCellularAutomatonConfig(directory) && success;
    success = SaveFoodConfig(directory) && success;
    success = SaveEnemiesConfig(directory) && success;

    if (m_tileManager) {
        std::string tilesJsonPath = directory + "/tiles.json";
        if (!m_tileManager->SaveToFile(tilesJsonPath)) {
            Logger::Log("ERROR: Failed to save tiles.json");
            success = false;
        }
        else {
            Logger::Log("Saved tiles.json to: " + tilesJsonPath);
        }
    }

    if (success) {
        Logger::Log("=== ALL CONFIGURATIONS SAVED SUCCESSFULLY ===");
    }
    else {
        Logger::Log("=== SOME CONFIGURATIONS FAILED TO SAVE ===");
    }

    return success;
}

bool WorldEditor::SaveWorldConfig(const std::string& directory) {
    std::string worldConfigPath = directory + "/world_gen.cfg";
    std::ofstream file(worldConfigPath);

    if (!file.is_open()) {
        Logger::Log("ERROR: Cannot open world config file: " + worldConfigPath);
        return false;
    }

    file << "Width=" << m_config.GetWidth() << "\n";
    file << "Height=" << m_config.GetHeight() << "\n";
    file << "Seed=" << m_config.GetSeed() << "\n";
    file << "NoiseFrequency=" << m_config.GetNoiseFrequency() << "\n";
    file << "NeighborRadius=" << m_config.GetNeighborRadius() << "\n";
    file << "GenerationMode=" << (m_config.GetRandomGeneration() ? "1" : "2") << "\n";
    file << "WorldName=" << m_config.GetWorldName() << "\n";
    file << "PlayerStartX=" << m_config.GetPlayerStartX() << "\n";
    file << "PlayerStartY=" << m_config.GetPlayerStartY() << "\n";
    file << "PlayerMaxHP=" << m_config.GetPlayerMaxHP() << "\n";
    file << "PlayerMaxHunger=" << m_config.GetPlayerMaxHunger() << "\n";
    file << "EnableHP=" << (m_config.GetEnableHP() ? "true" : "false") << "\n";
    file << "EnableHunger=" << (m_config.GetEnableHunger() ? "true" : "false") << "\n";
    file << "EnableEnemies=" << (m_config.GetEnableEnemies() ? "true" : "false") << "\n";
    file << "EnemySpawnRate=" << m_config.GetEnemySpawnRate() << "\n";

    if (!m_config.GetSurvivalRules().empty()) {
        file << "SurvivalRules=" << m_config.GetSurvivalRules() << "\n";
    }
    if (!m_config.GetBirthRules().empty()) {
        file << "BirthRules=" << m_config.GetBirthRules() << "\n";
    }
    if (!m_config.GetDeathRules().empty()) {
        file << "DeathRules=" << m_config.GetDeathRules() << "\n";
    }

    file.close();
    Logger::Log("Saved world config to: " + worldConfigPath);
    return true;
}

bool WorldEditor::SavePlayerConfig(const std::string& directory) {
    std::string playerConfigPath = directory + "/player.cfg";
    std::ofstream file(playerConfigPath);

    if (!file.is_open()) {
        Logger::Log("ERROR: Cannot open player config file: " + playerConfigPath);
        return false;
    }

    file << "DefaultPlayerX=" << m_config.GetPlayerStartX() << "\n";
    file << "DefaultPlayerY=" << m_config.GetPlayerStartY() << "\n";
    file << "MAX_HP=" << m_config.GetPlayerMaxHP() << "\n";
    file << "MAX_HUNGER=" << m_config.GetPlayerMaxHunger() << "\n";
    file << "EnableHP=" << (m_config.GetEnableHP() ? "true" : "false") << "\n";
    file << "EnableHunger=" << (m_config.GetEnableHunger() ? "true" : "false") << "\n";
    file << "BaseXP=100\n";
    file << "XPMultiplier=1.5\n";
    file << "MoveCooldownMs=50\n";

    file.close();
    Logger::Log("Saved player config to: " + playerConfigPath);
    return true;
}

bool WorldEditor::SaveTilesConfig(const std::string& directory) {
    std::string tilesConfigPath = directory + "/tiles.json";

    if (!m_tileManager->SaveToFile(tilesConfigPath)) {
        Logger::Log("ERROR: Failed to save tiles config");
        return false;
    }

    std::string spawnConfigPath = directory + "/world_spawn.cfg";
    std::ofstream spawnFile(spawnConfigPath);

    if (!spawnFile.is_open()) {
        Logger::Log("ERROR: Cannot open spawn config file: " + spawnConfigPath);
        return false;
    }

    const auto& allTiles = m_tileManager->GetAllTiles();
    for (const auto& pair : allTiles) {
        const TileType& tile = pair.second;

        if (tile.GetLowlandProbability() > 0 || tile.GetPlainsProbability() > 0 || tile.GetMountainProbability() > 0) {
            spawnFile << tile.GetCharacter() << "="
                << tile.GetLowlandProbability() << ":"
                << tile.GetPlainsProbability() << ":"
                << tile.GetMountainProbability() << "\n";
        }
    }

    spawnFile.close();
    Logger::Log("Saved tiles config to: " + directory);
    return true;
}

bool WorldEditor::SaveCellularAutomatonConfig(const std::string& directory) {
    std::string automatonConfigPath = directory + "/cellular_automaton.cfg";
    std::ofstream file(automatonConfigPath);

    if (!file.is_open()) {
        Logger::Log("ERROR: Cannot open automaton config file: " + automatonConfigPath);
        return false;
    }

    if (!m_config.GetSurvivalRules().empty()) {
        file << ".\n";
        file << "survival=" << m_config.GetSurvivalRules() << "\n";
    }

    if (!m_config.GetBirthRules().empty()) {
        file << "birth=" << m_config.GetBirthRules() << "\n";
    }

    if (!m_config.GetDeathRules().empty()) {
        file << "death=" << m_config.GetDeathRules() << "\n";
    }

    file.close();
    Logger::Log("Saved cellular automaton config to: " + automatonConfigPath);
    return true;
}

bool WorldEditor::SaveFoodConfig(const std::string& directory) {
    std::string foodConfigPath = directory + "/food.cfg";
    std::ofstream file(foodConfigPath);

    if (!file.is_open()) {
        Logger::Log("ERROR: Cannot open food config file: " + foodConfigPath);
        return false;
    }

    file << "# Food configuration\n";
    file << "# To be implemented\n";
    file << "FoodSpawnRate=10\n";
    file << "FoodNutrition=20\n";

    file.close();
    Logger::Log("Saved food config to: " + foodConfigPath);
    return true;
}

bool WorldEditor::SaveEnemiesConfig(const std::string& directory) {
    std::string enemiesConfigPath = directory + "/enemies.cfg";
    std::ofstream file(enemiesConfigPath);

    if (!file.is_open()) {
        Logger::Log("ERROR: Cannot open enemies config file: " + enemiesConfigPath);
        return false;
    }

    file << "# Enemies configuration\n";
    file << "EnableEnemies=" << (m_config.GetEnableEnemies() ? "true" : "false") << "\n";
    file << "EnemySpawnRate=" << m_config.GetEnemySpawnRate() << "\n";
    file << "EnemyDamage=10\n";
    file << "EnemyHP=50\n";

    file.close();
    Logger::Log("Saved enemies config to: " + enemiesConfigPath);
    return true;
}

bool WorldEditor::LoadFromTemplate(int templateSlot) {
    TemplateSystem templateSystem;
    if (!templateSystem.Initialize()) {
        Logger::Log("ERROR: Failed to initialize TemplateSystem");
        return false;
    }

    if (templateSystem.LoadTemplate(templateSlot, m_config)) {
        Logger::Log("Successfully loaded template " + std::to_string(templateSlot));

        return true;
    }

    Logger::Log("ERROR: Failed to load template " + std::to_string(templateSlot));
    return false;
}

bool WorldEditor::LoadTemplateConfig(const WorldConfig& config) {
    m_config = config;
    Logger::Log("Loaded template configuration: " + config.GetWorldName());
    return true;
}

void WorldEditor::RegisterHelpSystemEntries() {
    auto& helpSystem = HelpSystem::GetInstance();

    helpSystem.RegisterWorldTabHelp();
    helpSystem.RegisterPlayerTabHelp();
    helpSystem.RegisterTilesTabHelp();
    helpSystem.RegisterCommonElementsHelp();

    helpSystem.RegisterEditorTabHelp(); 
    helpSystem.RegisterEditorButtonsHelp();
}

std::string WorldEditor::GetCurrentFieldName() const {
    switch (m_currentTab) {
    case EditorTab::WORLD: {
        std::vector<std::string> fieldNames = {
            "World Name: ",
            "Width: ",
            "Height: ",
            "Random Generation: ",
            "Seed: ",
            "Noise Frequency: ",
            "Neighbor Radius: "
        };

        int visibleIndex = 0;
        for (int i = 0; i < 7; ++i) {
            if (i == 4 && !ShouldShowSeedField()) {
                continue;
            }
            if (visibleIndex == m_selectedField) {
                return fieldNames[i];
            }
            visibleIndex++;
        }
        break;
    }
    case EditorTab::PLAYER: {
        std::vector<std::string> fieldNames = {
            "Start X: ",
            "Start Y: ",
            "Max HP: ",
            "Max Hunger: ",
            "Enable HP: ",
            "Enable Hunger: "
        };
        if (m_selectedField < fieldNames.size()) {
            return fieldNames[m_selectedField];
        }
        break;
    }
    case EditorTab::TILES: {
        if (m_tilesState == TilesState::EDITING_TILE || m_tilesState == TilesState::ADDING_TILE) {
            std::vector<std::string> fieldNames = {
                "Symbol: ",
                "Color: ",
                "Name: ",
                "Lowland Probability: ",
                "Plains Probability: ",
                "Mountain Probability: "
            };
            if (m_selectedField < fieldNames.size()) {
                return fieldNames[m_selectedField];
            }
        }
        break;
    }
    case EditorTab::CELLULAR_AUTOMATON: {
        std::vector<std::string> fieldNames = {
            "Survival Rules: ",
            "Birth Rules: ",
            "Death Rules: "
        };
        if (m_selectedField < fieldNames.size()) {
            return fieldNames[m_selectedField];
        }
        break;
    }
    case EditorTab::ENEMIES: {
        std::vector<std::string> fieldNames = {
            "Enable Enemies: ",
            "Enemy Spawn Rate: "
        };
        if (m_selectedField < fieldNames.size()) {
            return fieldNames[m_selectedField];
        }
        break;
    }
    default:
        break;
    }
    return "";
}

std::string WorldEditor::GetCurrentButtonName() const {
    if (m_currentTab == EditorTab::TILES) {
        if (m_tilesState == TilesState::TILE_ACTIONS) {
            switch (m_tileActionIndex) {
            case 0: return "- Edit";
            case 1: return "- Delete";
            case 2: return "- Back";
            }
        }
        else if (m_tilesState == TilesState::EDITING_TILE || m_tilesState == TilesState::ADDING_TILE) {
            int fieldCount = 6;
            if (m_selectedField == fieldCount) return "Save";
            if (m_selectedField == fieldCount + 1) return "Cancel";
        }
    }

    if (m_selectedButton == 1) {
        return m_editorMode == EditorMode::CREATE_WORLD ? "CREATE" : "SAVE TEMPLATE";
    }
    else if (m_selectedButton == 2) {
        return "BACK";
    }

    return "";
}

std::string WorldEditor::GetCurrentTabName() const {
    switch (m_currentTab) {
    case EditorTab::WORLD: return "World";
    case EditorTab::PLAYER: return "Player";
    case EditorTab::TILES: return "Tiles";
    case EditorTab::CELLULAR_AUTOMATON: return "Cellular Automaton";
    case EditorTab::FOOD: return "Food";
    case EditorTab::ENEMIES: return "Enemies";
    case EditorTab::WIN: return "Win";
    case EditorTab::LOSE: return "Lose";
    default: return "";
    }
}

void WorldEditor::UpdateHelpForCurrentSelection() {
    auto& helpSystem = HelpSystem::GetInstance();
    std::string currentItemId;

    if (m_isEditingText && m_editingField >= 0) {
        currentItemId = GetCurrentFieldName();
    }
    else if (m_currentTab == EditorTab::TILES) {
        if (m_tilesState == TilesState::MAIN_LIST) {
            if (!m_availableTileIds.empty() &&
                m_selectedField < static_cast<int>(m_availableTileIds.size())) {
                currentItemId = "Tile List Item";
            }
            else if (m_selectedField == static_cast<int>(m_availableTileIds.size())) {
                currentItemId = "+ Add New Tile";
            }
        }
        else if (m_tilesState == TilesState::TILE_ACTIONS) {
            currentItemId = GetCurrentButtonName();
        }
        else if (m_tilesState == TilesState::EDITING_TILE ||
            m_tilesState == TilesState::ADDING_TILE) {
            currentItemId = GetCurrentFieldName();
        }
    }
    else {
        currentItemId = GetCurrentFieldName();

        if (currentItemId.empty() && m_selectedButton > 0) {
            currentItemId = GetCurrentButtonName();
        }
    }

    if (!currentItemId.empty()) {
        std::string helpText = helpSystem.GetHelpForItem(currentItemId);
        if (!helpText.empty()) {
            HelpPanel::SetHelpText(helpText);
        }
        else {
            currentItemId = GetCurrentTabName();
            helpText = helpSystem.GetHelpForItem(currentItemId);
            if (!helpText.empty()) {
                HelpPanel::SetHelpText(helpText);
            }
            else {
                HelpPanel::ClearHelpText();
            }
        }
    }
    else {
        currentItemId = GetCurrentTabName();
        std::string helpText = helpSystem.GetHelpForItem(currentItemId);
        if (!helpText.empty()) {
            HelpPanel::SetHelpText(helpText);
        }
        else {
            HelpPanel::ClearHelpText();
        }
    }
}