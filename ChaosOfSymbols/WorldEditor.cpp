#include "WorldEditor.h"
#include <windows.h>
#include <set>
#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>
#include "TemplateSystem.h"
#include <sstream>
#include "Logger.h"
#include "HelpPanel.h"
#include "HelpSystem.h"
#include <filesystem>
#include "FoodManager.h"

namespace fs = std::filesystem;

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
    , m_foodState(FoodState::MAIN_LIST)
    , m_prevFoodState(FoodState::MAIN_LIST)
    , m_selectedFoodIndex(0)
    , m_foodActionIndex(0)
    , m_newFoodName("new_food")
    , m_newFoodSymbol('*')
    , m_newFoodColor(10) // Зеленый
    , m_newHungerRestore(10)
    , m_newHpRestore(5)
    , m_newSpawnWeight(1)
    , m_newExperience(5)
    , m_cellularState(CellularAutomatonState::MAIN_LIST)
    , m_prevCellularState(CellularAutomatonState::MAIN_LIST)
    , m_selectedRuleType(0)
    , m_editingRuleIndex(0)
    , m_editingRule(false)
    , m_selectedTileForRules('\0')
    , m_cellularScrollOffset(0)
    , m_visibleRulesCount(7)
    , m_cursorPos(0)
{
    HelpPanel::Initialize();
    RegisterHelpSystemEntries();

    if (m_editorMode == EditorMode::CREATE_TEMPLATE) {
        m_tilesConfigPath = "templates/template" + std::to_string(slot) + "/tiles.json";

        TemplateSystem templateSystem;
        if (templateSystem.Initialize()) {
            if (templateSystem.LoadTemplate(slot, m_config)) {
                Logger::Log("Loaded existing template configuration for slot " + std::to_string(slot));

                std::string templateDir = "templates/template" + std::to_string(slot);
                std::string spawnConfigPath = templateDir + "/world_spawn.cfg";

                std::ifstream spawnFile(spawnConfigPath);
                if (spawnFile.is_open()) {
                    std::string line;
                    std::unordered_map<char, std::vector<float>> tileProbabilities;

                    while (std::getline(spawnFile, line)) {
                        if (line.empty() || line[0] == '#') continue;

                        size_t delimiterPos = line.find('=');
                        if (delimiterPos != std::string::npos) {
                            char tileChar = line[0];
                            std::string probabilitiesStr = line.substr(delimiterPos + 1);

                            std::vector<float> probs;
                            std::stringstream probStream(probabilitiesStr);
                            std::string probToken;

                            while (std::getline(probStream, probToken, ':')) {
                                try {
                                    probs.push_back(std::stof(probToken));
                                }
                                catch (...) {
                                    probs.push_back(0.1f);
                                }
                            }
                            while (probs.size() < 3) {
                                probs.push_back(0.1f);
                            }

                            tileProbabilities[tileChar] = probs;
                        }
                    }
                    spawnFile.close();
                    m_config.SetTileProbabilities(tileProbabilities);
                }
            }
            else {
                Logger::Log("No existing template found for slot " + std::to_string(slot));

                ResetTemplateData();

                std::string templateDir = "templates/template" + std::to_string(slot);
                if (fs::exists(templateDir)) {
                    Logger::Log("Template directory exists but template is invalid, cleaning...");
                    ClearTemplate();
                }
            }
        }
    }
    else {
        m_tilesConfigPath = "saves/slot" + std::to_string(slot) + "/tiles.json";

        std::string savePath = "saves/slot" + std::to_string(slot);

        if (fs::exists(savePath + "/world_gen.cfg")) {
            if (m_config.LoadFromDirectory(savePath)) {
                Logger::Log("Loaded existing world configuration for slot " + std::to_string(slot));
            }
        }
    }

    CreateDirectoryForSlot(slot);

    m_inputManager = std::make_unique<InputManager>();

    Logger::Log("Creating TileTypeManager with path: " + m_tilesConfigPath);
    m_tileManager = std::make_unique<TileTypeManager>(m_tilesConfigPath);

    std::ifstream file(m_tilesConfigPath);
    if (file.good()) {
        file.close();
        Logger::Log("Tile config exists: " + m_tilesConfigPath);
        if (m_tileManager->LoadFromFile()) {
            Logger::Log("Successfully loaded tiles from: " + m_tilesConfigPath);
        }
        else {
            Logger::Log("WARNING: Failed to load tiles from: " + m_tilesConfigPath);
        }
    }
    else {
        Logger::Log("Tile config not found: " + m_tilesConfigPath);
        Logger::Log("Will create new config when tiles are added");
    }

    LoadAvailableTiles();
    LoadCellularAutomatonRules();

    m_newTileName = "new_tile";
    m_newTileSymbol = 'A';
    m_newTileColor = 7;
    m_newTileLowlandProb = 10;
    m_newTilePlainsProb = 10;
    m_newTileMountainProb = 10;

    if (m_editorMode == EditorMode::CREATE_TEMPLATE) {
        m_tilesConfigPath = "templates/template" + std::to_string(slot) + "/tiles.json";
        m_foodConfigPath = "templates/template" + std::to_string(slot) + "/food.cfg";
    }
    else {
        m_tilesConfigPath = "saves/slot" + std::to_string(slot) + "/tiles.json";
        m_foodConfigPath = "saves/slot" + std::to_string(slot) + "/food.cfg";
    }

    m_foodManager = std::make_unique<FoodManager>();

    std::ifstream foodFile(m_foodConfigPath);
    if (foodFile.good()) {
        foodFile.close();
        if (m_foodManager->LoadFromFile(m_foodConfigPath)) {
            Logger::Log("Successfully loaded food from: " + m_foodConfigPath);
            LoadAvailableFood();
        }
    }

    Logger::Log("WorldEditor initialized for slot " + std::to_string(slot) +
        " with mode: " + (m_editorMode == EditorMode::CREATE_WORLD ? "CREATE_WORLD" : "CREATE_TEMPLATE") +
        " and " + std::to_string(m_availableTileIds.size()) + " user tiles");
}

void WorldEditor::CreateDefaultTiles() {
    if (!m_tileManager) return;

    m_tileManager->RegisterTileType(TileType(0, "air", ' ', 0, true, 0, 0, 0));
    m_tileManager->RegisterTileType(TileType(1, "grass", '.', 10, true, 10, 80, 10));
    m_tileManager->RegisterTileType(TileType(2, "stone_wall", '#', 8, false, 0, 0, 0));
    m_tileManager->RegisterTileType(TileType(3, "water", '~', 9, false, 80, 20, 0));
    m_tileManager->RegisterTileType(TileType(4, "mountain", '^', 7, false, 0, 10, 80));

    m_tileManager->SaveToFile();

    Logger::Log("Created default tiles for slot " + std::to_string(m_slot));
}

void WorldEditor::CreateDirectoryForSlot(int slot) {
    if (m_editorMode == EditorMode::CREATE_WORLD) {
        std::string dirPath = "saves/slot" + std::to_string(slot);
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

        std::string templateDir = tempDir + "/template" + std::to_string(slot);
#ifdef _WIN32
        CreateDirectoryA(templateDir.c_str(), NULL);
#else
        mkdir(templateDir.c_str(), 0777);
#endif
        std::string infoFile = templateDir + "/template_info.txt";
        if (!fs::exists(infoFile)) {
            Logger::Log("New template detected, resetting data");
            ResetTemplateData();
        }
        else {
            Logger::Log("Existing template detected, keeping data");
        }
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

    Logger::Log("Render() - needFullRedraw: " + std::to_string(m_needFullRedraw) +
        ", prevTab: " + std::to_string(static_cast<int>(m_prevTab)) +
        ", currentTab: " + std::to_string(static_cast<int>(m_currentTab)));

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

    Logger::Log("Render() - после отрисовки, needFullRedraw установлен в: " + std::to_string(m_needFullRedraw));
}

bool WorldEditor::NeedsRedraw() const {
    bool needs = m_prevSelectedField != m_selectedField ||
        m_prevSelectedButton != m_selectedButton ||
        m_isEditingText ||
        m_currentTab != m_prevTab;

    Logger::Log("NeedsRedraw() возвращает: " + std::to_string(needs) +
        " (prevField: " + std::to_string(m_prevSelectedField) +
        ", currField: " + std::to_string(m_selectedField) +
        ", prevButton: " + std::to_string(m_prevSelectedButton) +
        ", currButton: " + std::to_string(m_selectedButton) + ")");

    return needs;
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
    }

    RenderBottomButtons();

    SetConsoleTextAttribute(hConsole, 7);
}

void WorldEditor::RenderTabHeader() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    rlutil::locate(2, 3);

    std::vector<std::string> tabNames = {
        "World", "Player", "Tiles", "Cellular Automaton", "Food"
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
    if ((m_currentTab == EditorTab::TILES &&
        (m_tilesState == TilesState::EDITING_TILE ||
            m_tilesState == TilesState::ADDING_TILE ||
            m_tilesState == TilesState::TILE_ACTIONS)) ||
        (m_currentTab == EditorTab::FOOD &&
            (m_foodState == FoodState::EDITING_FOOD ||
                m_foodState == FoodState::ADDING_FOOD ||
                m_foodState == FoodState::FOOD_ACTIONS))) {
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
        std::cout << "  No user tiles available. Add your first tile to get started.";
        line += 2;

        rlutil::locate(4, line);
        std::cout << "  NOTE: World has system tiles (air, border) that are hidden from view.";
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

    if (m_selectedTileIndex >= 0 && m_selectedTileIndex < static_cast<int>(m_availableTileIds.size())) {
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
    bool deleteSelected = (m_tileActionIndex == 1);
    bool backSelected = (m_tileActionIndex == 2);

    rlutil::locate(6, line);
    if (editSelected) {
        SetConsoleTextAttribute(hConsole, 10);
        std::cout << "> ";
    }
    else {
        SetConsoleTextAttribute(hConsole, 7);
        std::cout << "  ";
    }
    std::cout << "- Edit";
    line++;

    rlutil::locate(6, line);
    if (deleteSelected) {
        SetConsoleTextAttribute(hConsole, 10);
        std::cout << "> ";
    }
    else {
        SetConsoleTextAttribute(hConsole, 7);
        std::cout << "  ";
    }
    std::cout << "- Delete";
    line++;

    rlutil::locate(6, line);
    if (backSelected) {
        SetConsoleTextAttribute(hConsole, 10);
        std::cout << "> ";
    }
    else {
        SetConsoleTextAttribute(hConsole, 7);
        std::cout << "  ";
    }
    std::cout << "- Back";

    SetConsoleTextAttribute(hConsole, 7);
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
        m_needFullRedraw = true;
    }
    else if (m_inputManager->IsMenuDown()) {
        m_tileActionIndex = (m_tileActionIndex + 1) % 3;
        m_needFullRedraw = true;
    }
    else if (m_inputManager->IsMenuSelect()) {
        switch (m_tileActionIndex) {
        case 0: // Edit
            if (m_selectedTileIndex >= 0 && m_selectedTileIndex < static_cast<int>(m_availableTileIds.size())) {
                StartEditingTile();
            }
            break;
        case 1: // Delete
            DeleteSelectedTile();
            m_tilesState = TilesState::MAIN_LIST;
            m_selectedField = min(m_selectedField, static_cast<int>(m_availableTileIds.size() - 1));
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
            if (m_tilesState == TilesState::ADDING_TILE) {
                m_newTileName = "new_tile";
                m_newTileSymbol = 'A';
                m_newTileColor = 7;
                m_newTileLowlandProb = 10;
                m_newTilePlainsProb = 10;
                m_newTileMountainProb = 10;
            }
            m_tilesState = TilesState::MAIN_LIST;
            m_selectedField = (m_tilesState == TilesState::MAIN_LIST && m_availableTileIds.empty()) ? 0 :
                (m_selectedTileIndex < static_cast<int>(m_availableTileIds.size()) ? m_selectedTileIndex : 0);
            m_selectedButton = 0;
            m_needFullRedraw = true;
        }
    }
    else if (m_inputManager->IsMenuBack()) {
        if (m_tilesState == TilesState::ADDING_TILE) {
            m_newTileName = "new_tile";
            m_newTileSymbol = 'A';
            m_newTileColor = 7;
            m_newTileLowlandProb = 10;
            m_newTilePlainsProb = 10;
            m_newTileMountainProb = 10;
        }
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
                m_tempTileStringInput = std::string(1, m_editedTileSymbol);
                break;
            case 1: // Color
                m_tempTileStringInput = std::to_string(m_editedTileColor);
                break;
            case 2: // Name
                m_tempTileStringInput = m_editedTileName;
                break;
            case 3: // Lowland Probability
                m_tempTileStringInput = std::to_string(m_editedTileLowlandProb);
                break;
            case 4: // Plains Probability
                m_tempTileStringInput = std::to_string(m_editedTilePlainsProb);
                break;
            case 5: // Mountain Probability
                m_tempTileStringInput = std::to_string(m_editedTileMountainProb);
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
    // Используем ту же расширенную логику для символов тайлов
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

    if (m_inputManager->IsKeyPressed(VK_BACK) && !m_tempTileStringInput.empty()) {
        m_tempTileStringInput.clear();
        m_needFullRedraw = true;
        return;
    }

    if (m_inputManager->IsKeyPressed(VK_DELETE)) {
        m_tempTileStringInput.clear();
        m_needFullRedraw = true;
        return;
    }

    bool shiftPressed = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    bool capsLockOn = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;

    // 1. Цифры
    for (char c = '0'; c <= '9'; c++) {
        if (m_inputManager->IsKeyPressed(c)) {
            if (shiftPressed) {
                static const std::unordered_map<char, char> shiftDigits = {
                    {'0', ')'}, {'1', '!'}, {'2', '@'}, {'3', '#'}, {'4', '$'},
                    {'5', '%'}, {'6', '^'}, {'7', '&'}, {'8', '*'}, {'9', '('}
                };
                auto it = shiftDigits.find(c);
                if (it != shiftDigits.end()) {
                    m_tempTileStringInput = std::string(1, it->second);
                    m_needFullRedraw = true;
                }
            }
            else {
                m_tempTileStringInput = std::string(1, c);
                m_needFullRedraw = true;
            }
            return;
        }
    }

    // 2. Буквы
    for (int vkCode = 'A'; vkCode <= 'Z'; vkCode++) {
        if (m_inputManager->IsKeyPressed(vkCode)) {
            char c = static_cast<char>(vkCode);
            if (shiftPressed ^ capsLockOn) {
                m_tempTileStringInput = std::string(1, c);
            }
            else {
                m_tempTileStringInput = std::string(1, c + 32);
            }
            m_needFullRedraw = true;
            return;
        }
    }

    // 3. Специальные символы
    static const std::unordered_map<int, std::pair<char, char>> specialKeys = {
        {VK_OEM_MINUS, {'-', '_'}},
        {VK_OEM_PLUS, {'=', '+'}},
        {VK_OEM_1, {';', ':'}},
        {VK_OEM_2, {'/', '?'}},
        {VK_OEM_3, {'`', '~'}},
        {VK_OEM_4, {'[', '{'}},
        {VK_OEM_5, {'\\', '|'}},
        {VK_OEM_6, {']', '}'}},
        {VK_OEM_7, {'\'', '"'}},
        {VK_OEM_COMMA, {',', '<'}},
        {VK_OEM_PERIOD, {'.', '>'}},
        {VK_SPACE, {' ', ' '}},
    };

    for (const auto& [vkCode, chars] : specialKeys) {
        if (m_inputManager->IsKeyPressed(vkCode)) {
            char symbol = shiftPressed ? chars.second : chars.first;
            m_tempTileStringInput = std::string(1, symbol);
            m_needFullRedraw = true;
            return;
        }
    }

    // 4. Другие символы
    for (int i = 32; i <= 255; i++) {
        if (m_inputManager->IsKeyPressed(i)) {
            if ((i >= '0' && i <= '9') || (i >= 'A' && i <= 'Z')) {
                continue;
            }

            bool isSpecialVK = false;
            for (const auto& [vkCode, _] : specialKeys) {
                if (vkCode == i) {
                    isSpecialVK = true;
                    break;
                }
            }
            if (isSpecialVK) {
                continue;
            }

            m_tempTileStringInput = std::string(1, static_cast<char>(i));
            m_needFullRedraw = true;
            return;
        }
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
    // Ctrl+C - копирование в буфер обмена
    if (IsCtrlPressed() && m_inputManager->IsKeyPressed('C')) {
        if (!m_tempTileStringInput.empty()) {
            CopyToClipboard(m_tempTileStringInput);
            Logger::Log("Copied tile name to clipboard: " + m_tempTileStringInput);
        }
        return;
    }

    // Ctrl+V - вставка из буфера обмена
    if (IsCtrlPressed() && m_inputManager->IsKeyPressed('V')) {
        std::string clipboardText = PasteFromClipboard();
        if (!clipboardText.empty()) {
            m_tempTileStringInput += clipboardText;
            Logger::Log("Pasted tile name from clipboard: " + clipboardText);
            m_needFullRedraw = true;
        }
        return;
    }

    // Обычные символы (только если не нажат Ctrl)
    for (char c = '0'; c <= '9'; c++) {
        if (m_inputManager->IsKeyPressed(c) && !IsCtrlPressed()) {
            m_tempTileStringInput += c;
            m_needFullRedraw = true;
            return;
        }
    }

    for (char c = 'A'; c <= 'Z'; c++) {
        if (m_inputManager->IsKeyPressed(c) && !IsCtrlPressed()) {
            m_tempTileStringInput += c;
            m_needFullRedraw = true;
            return;
        }
    }

    for (char c = 'a'; c <= 'z'; c++) {
        if (m_inputManager->IsKeyPressed(c) && !IsCtrlPressed()) {
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

    if (m_inputManager->IsKeyPressed(VK_DELETE)) {
        m_tempTileStringInput.clear();
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

        // Сохраняем старый символ для обновления правил
        char oldSymbol = tile->GetCharacter();
        char newSymbol = m_editedTileSymbol;

        try {
            TileType newTile(tileId, m_editedTileName, m_editedTileSymbol, m_editedTileColor,
                true, m_editedTileLowlandProb, m_editedTilePlainsProb, m_editedTileMountainProb);

            m_tileManager->RegisterTileType(newTile);

            // Если символ изменился, обновляем правила клеточного автомата
            if (oldSymbol != newSymbol) {
                UpdateCellularRulesForTile(oldSymbol, newSymbol);
            }

            m_tileManager->SaveToFile();

            LoadAvailableTiles();

            Logger::Log("Tile updated: " + m_editedTileName + " (ID: " + std::to_string(tileId) + ")" +
                " Symbol: '" + std::string(1, oldSymbol) + "' -> '" + std::string(1, newSymbol) + "'" +
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
        Logger::Log("=== ADDING NEW TILE ===");
        Logger::Log("New tile ID: " + std::to_string(newId));
        Logger::Log("Name: " + m_newTileName);
        Logger::Log("Symbol: '" + std::string(1, m_newTileSymbol) + "'");
        Logger::Log("Color: " + std::to_string(m_newTileColor));
        Logger::Log("Probabilities: L=" + std::to_string(m_newTileLowlandProb) +
            " P=" + std::to_string(m_newTilePlainsProb) +
            " M=" + std::to_string(m_newTileMountainProb));

        // Проверяем уникальность символа
        bool symbolExists = false;
        char existingSymbol = '\0';
        const auto& allTiles = m_tileManager->GetAllTiles();
        for (const auto& pair : allTiles) {
            if (pair.second.GetCharacter() == m_newTileSymbol) {
                symbolExists = true;
                existingSymbol = pair.second.GetCharacter();
                Logger::Log("WARNING: Symbol '" + std::string(1, m_newTileSymbol) +
                    "' already exists in tile: " + pair.second.GetName());

                // Не прерываем выполнение, пользователь может захотеть заменить символ
                // Просто предупреждаем в логах
                break;
            }
        }

        TileType newTile(newId, m_newTileName, m_newTileSymbol, m_newTileColor,
            true, m_newTileLowlandProb, m_newTilePlainsProb, m_newTileMountainProb);

        m_tileManager->RegisterTileType(newTile);
        Logger::Log("Tile registered in manager");

        // Если символ уже существует, обновляем правила для этого символа
        if (symbolExists) {
            // Находим старый тайл с таким символом и обновляем его правила
            // Но в данном случае, поскольку мы добавляем новый тайл, возможно,
            // пользователь хочет заменить старый тайл новым с теми же правилами
            // или создать конфликт. Лучше просто предупредить.
            Logger::Log("NOTE: Symbol '" + std::string(1, m_newTileSymbol) +
                "' already exists. Cellular automaton rules for this symbol will be preserved.");
        }
        else {
            // Добавляем пустые правила для нового символа
            m_survivalRules[m_newTileSymbol] = "";
            m_birthRules[m_newTileSymbol] = "";
            m_deathRules[m_newTileSymbol] = "";
        }

        if (m_tileManager->SaveToFile()) {
            Logger::Log("Tile saved to file successfully");
        }
        else {
            Logger::Log("ERROR: Failed to save tile to file");
        }

        Logger::Log("Reloading available tiles...");
        LoadAvailableTiles();

        Logger::Log("Added new tile: " + m_newTileName + " (ID: " + std::to_string(newId) +
            ") Symbol: '" + std::string(1, m_newTileSymbol) + "'" +
            " L:" + std::to_string(m_newTileLowlandProb) +
            " P:" + std::to_string(m_newTilePlainsProb) +
            " M:" + std::to_string(m_newTileMountainProb));

        // Сохраняем правила клеточного автомата
        SaveCellularAutomatonRules();

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

        m_tilesState = TilesState::MAIN_LIST;
        m_selectedField = 0;
        m_selectedButton = 0;
        m_needFullRedraw = true;

        Logger::Log("=== NEW TILE ADDED ===");
    }
    catch (const std::exception& e) {
        Logger::Log("ERROR: Failed to add new tile: " + std::string(e.what()));
    }
}

void WorldEditor::DeleteSelectedTile() {
    if (m_availableTileIds.empty() || m_selectedTileIndex >= static_cast<int>(m_availableTileIds.size())) return;

    int tileId = m_availableTileIds[m_selectedTileIndex];

    if (tileId == 0 || tileId == -1 || tileId == -2) {
        Logger::Log("Cannot delete system tiles (air/border)");
        return;
    }

    TileType* tile = m_tileManager->GetTileType(tileId);
    if (!tile) return;

    if (tile->GetName() == "air" || tile->GetName() == "border" ||
        tile->GetName() == "stone_wall" || tile->GetCharacter() == ' ' ||
        tile->GetCharacter() == '#') {
        Logger::Log("Cannot delete system tiles (air/border)");
        return;
    }

    char tileChar = tile->GetCharacter();

    m_tileManager->RemoveTileType(tileId);

    // Удаляем правила для этого символа
    m_survivalRules.erase(tileChar);
    m_birthRules.erase(tileChar);
    m_deathRules.erase(tileChar);

    m_tileManager->SaveToFile();

    LoadAvailableTiles();

    if (m_selectedTileIndex >= static_cast<int>(m_availableTileIds.size())) {
        m_selectedTileIndex = max(0, static_cast<int>(m_availableTileIds.size()) - 1);
    }

    // Сохраняем обновленные правила
    SaveCellularAutomatonRules();

    Logger::Log("Deleted user tile with ID: " + std::to_string(tileId) +
        " and symbol: '" + std::string(1, tileChar) + "'");
}

void WorldEditor::LoadAvailableTiles() {
    Logger::Log("=== LOADING AVAILABLE TILES ===");

    m_availableTileIds.clear();

    if (!m_tileManager) {
        Logger::Log("ERROR: No tile manager");
        return;
    }

    if (!m_tileManager->LoadFromFile(m_tilesConfigPath)) {
        Logger::Log("ERROR: Failed to load tiles from: " + m_tilesConfigPath);
        return;
    }

    const auto& allTiles = m_tileManager->GetAllTiles();
    Logger::Log("Tile manager has " + std::to_string(allTiles.size()) + " tiles");

    for (const auto& pair : allTiles) {
        int tileId = pair.first;
        const TileType& tile = pair.second;

        Logger::Log("Found tile ID " + std::to_string(tileId) +
            ": '" + std::string(1, tile.GetCharacter()) + "' - " +
            tile.GetName() + " (color: " + std::to_string(tile.GetColor()) + ")");
    }

    for (const auto& pair : allTiles) {
        int tileId = pair.first;
        const TileType& tile = pair.second;

        if (tileId < 0) {
            Logger::Log("Skipping system tile ID " + std::to_string(tileId) +
                ": '" + std::string(1, tile.GetCharacter()) + "' - " + tile.GetName());
            continue;
        }

        if (tile.GetCharacter() == ' ' || tile.GetCharacter() == 0) {
            Logger::Log("Skipping tile with empty character ID " + std::to_string(tileId) +
                ": " + tile.GetName());
            continue;
        }

        m_availableTileIds.push_back(tileId);
    }

    std::sort(m_availableTileIds.begin(), m_availableTileIds.end());

    Logger::Log("Loaded " + std::to_string(m_availableTileIds.size()) +
        " user-available tiles (system tiles filtered out)");

    std::string availableIds = "Available tile IDs: ";
    for (int id : m_availableTileIds) {
        availableIds += std::to_string(id) + " ";
    }
    Logger::Log(availableIds);

    UpdateCellularRulesFromTiles();
    LoadCellularAutomatonRules();

    Logger::Log("=== LOADING COMPLETE ===");
}

void WorldEditor::RenderCellularAutomatonTab() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    std::vector<char> tileChars;
    std::vector<TileType*> tileTypes;

    for (int tileId : m_availableTileIds) {
        TileType* tile = m_tileManager->GetTileType(tileId);
        if (tile && tile->GetCharacter() != ' ') {
            tileChars.push_back(tile->GetCharacter());
            tileTypes.push_back(tile);
        }
    }

    int startLine = 8;
    int currentLine = startLine;

    if (m_needFullRedraw) {
        for (int i = 5; i < 7; i++) {
            ClearLine(i);
        }

        rlutil::locate(2, 5);
        SetConsoleTextAttribute(hConsole, 14);
        std::cout << "Cellular Automaton Rules";
        SetConsoleTextAttribute(hConsole, 7);

        rlutil::locate(2, 6);
        std::cout << "------------------------------------------------------------";
    }

    if (tileChars.empty()) {
        if (m_needFullRedraw || m_selectedField != m_prevSelectedField ||
            m_selectedButton != m_prevSelectedButton) {
            ClearLine(currentLine);
            rlutil::locate(4, currentLine);
            std::cout << "No user tiles available. Add tiles in the Tiles tab first.";
        }
        currentLine++;
    }
    else {
        int selectedTileIndex = m_selectedField / 3;
        selectedTileIndex = std::clamp(selectedTileIndex, 0, (int)tileChars.size() - 1);
        char tileChar = tileChars[selectedTileIndex];
        TileType* tile = tileTypes[selectedTileIndex];

        if (tile) {
            bool tileInfoNeedsUpdate = m_needFullRedraw ||
                (selectedTileIndex != (m_prevSelectedField / 3));

            if (tileInfoNeedsUpdate) {
                ClearLine(currentLine);
                rlutil::locate(4, currentLine);

                SetConsoleTextAttribute(hConsole, tile->GetColor());
                std::cout << tileChar;

                SetConsoleTextAttribute(hConsole, 14);
                std::cout << " - " << tile->GetName();

                SetConsoleTextAttribute(hConsole, 7);

                int counterX = 78 - std::to_string(selectedTileIndex + 1).length() -
                    std::to_string(tileChars.size()).length() - 3;
                rlutil::locate(counterX, currentLine);
                SetConsoleTextAttribute(hConsole, 8);
                std::cout << "[" << (selectedTileIndex + 1) << "/" << tileChars.size() << "]";
                SetConsoleTextAttribute(hConsole, 7);
            }
            currentLine++;

            auto survivalIt = m_survivalRules.find(tileChar);
            auto birthIt = m_birthRules.find(tileChar);
            auto deathIt = m_deathRules.find(tileChar);

            std::string survivalRule = (survivalIt != m_survivalRules.end()) ? survivalIt->second : "";
            std::string birthRule = (birthIt != m_birthRules.end()) ? birthIt->second : "";
            std::string deathRule = (deathIt != m_deathRules.end()) ? deathIt->second : "";

            std::string rules[3] = { survivalRule, birthRule, deathRule };
            std::string ruleNames[3] = { "Survival Rules: ", "Birth Rules: ", "Death Rules: " };

            for (int ruleType = 0; ruleType < 3; ruleType++) {
                int absoluteRuleIndex = selectedTileIndex * 3 + ruleType;
                bool isSelected = (absoluteRuleIndex == m_selectedField);
                bool isEditing = (isSelected && m_editingRule);
                bool wasSelected = (absoluteRuleIndex == m_prevSelectedField);

                bool needsUpdate = m_needFullRedraw ||
                    (isSelected != wasSelected) ||
                    (selectedTileIndex != (m_prevSelectedField / 3)) ||
                    (isEditing && (m_tempRuleInput != m_prevRuleInput));

                if (needsUpdate) {
                    ClearLine(currentLine);
                    rlutil::locate(8, currentLine);

                    if (isSelected) {
                        if (isEditing) {
                            SetConsoleTextAttribute(hConsole, 11);
                            std::cout << "> ";
                        }
                        else {
                            SetConsoleTextAttribute(hConsole, 10);
                            std::cout << "> ";
                        }
                    }
                    else {
                        SetConsoleTextAttribute(hConsole, 7);
                        std::cout << "  ";
                    }

                    std::cout << ruleNames[ruleType];

                    if (isEditing) {
                        SetConsoleTextAttribute(hConsole, 11);

                        // Отображаем текст до курсора
                        if (m_cursorPos > 0) {
                            std::cout << m_tempRuleInput.substr(0, m_cursorPos);
                        }

                        // Отображаем курсор
                        std::cout << "_";

                        // Отображаем текст после курсора
                        if (m_cursorPos < m_tempRuleInput.length()) {
                            std::cout << m_tempRuleInput.substr(m_cursorPos);
                        }
                    }
                    else {
                        if (!rules[ruleType].empty()) {
                            std::cout << rules[ruleType];
                        }
                    }

                    SetConsoleTextAttribute(hConsole, 7);
                }
                currentLine++;
            }
        }
    }

    if (m_needFullRedraw) {
        for (int i = currentLine; i < 15; i++) {
            ClearLine(i);
        }
    }

    RenderBottomButtons();

    SetConsoleTextAttribute(hConsole, 7);
}

void WorldEditor::RenderCellularMainList(int startLine) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    int line = startLine;

    std::vector<char> tileChars;
    for (int tileId : m_availableTileIds) {
        TileType* tile = m_tileManager->GetTileType(tileId);
        if (tile && tile->GetCharacter() != ' ') {
            tileChars.push_back(tile->GetCharacter());
        }
    }

    if (tileChars.empty()) {
        rlutil::locate(4, line);
        std::cout << "No user tiles available. Add tiles in the Tiles tab first.";
        line += 2;
    }
    else {
        for (size_t i = 0; i < tileChars.size(); ++i) {
            char tileChar = tileChars[i];
            TileType* tile = nullptr;

            for (int tileId : m_availableTileIds) {
                TileType* t = m_tileManager->GetTileType(tileId);
                if (t && t->GetCharacter() == tileChar) {
                    tile = t;
                    break;
                }
            }

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
            std::cout << tileChar;

            SetConsoleTextAttribute(hConsole, isSelected ? 10 : 7);
            std::cout << " - " << tile->GetName();

            line++;

            rlutil::locate(8, line);
            std::cout << "Survival Rules: ";
            auto survivalIt = m_survivalRules.find(tileChar);
            if (survivalIt != m_survivalRules.end() && !survivalIt->second.empty()) {
                std::cout << survivalIt->second;
            }
            else {
                std::cout << "(not set)";
            }

            line++;
            rlutil::locate(8, line);
            std::cout << "Birth Rules: ";
            auto birthIt = m_birthRules.find(tileChar);
            if (birthIt != m_birthRules.end() && !birthIt->second.empty()) {
                std::cout << birthIt->second;
            }
            else {
                std::cout << "(not set)";
            }

            line++;
            rlutil::locate(8, line);
            std::cout << "Death Rules: ";
            auto deathIt = m_deathRules.find(tileChar);
            if (deathIt != m_deathRules.end() && !deathIt->second.empty()) {
                std::cout << deathIt->second;
            }
            else {
                std::cout << "(not set)";
            }

            line += 2;
        }
    }
}

void WorldEditor::RenderCellularTileSelection(int startLine) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    int line = startLine;

    std::vector<std::string> ruleTypes = {
        "Survival Rules",
        "Birth Rules",
        "Death Rules"
    };

    for (int i = 0; i < ruleTypes.size(); ++i) {
        bool isSelected = (i == m_selectedField);

        rlutil::locate(6, line + i);

        if (isSelected) {
            SetConsoleTextAttribute(hConsole, 10);
            std::cout << "> ";
        }
        else {
            SetConsoleTextAttribute(hConsole, 7);
            std::cout << "  ";
        }

        std::cout << ruleTypes[i];
    }
}

void WorldEditor::RenderCellularEditing(int startLine) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    int line = startLine;

    rlutil::locate(4, line);
    std::cout << "Editing ";

    std::string ruleTypeName;
    std::string* currentRule = nullptr;

    switch (m_selectedRuleType) {
    case 0:
        std::cout << "Survival Rules";
        ruleTypeName = "Survival Rules";
        currentRule = &m_survivalRules[m_selectedTileForRules];
        break;
    case 1:
        std::cout << "Birth Rules";
        ruleTypeName = "Birth Rules";
        currentRule = &m_birthRules[m_selectedTileForRules];
        break;
    case 2:
        std::cout << "Death Rules";
        ruleTypeName = "Death Rules";
        currentRule = &m_deathRules[m_selectedTileForRules];
        break;
    }

    line += 2;

    TileType* tile = nullptr;
    for (int tileId : m_availableTileIds) {
        TileType* t = m_tileManager->GetTileType(tileId);
        if (t && t->GetCharacter() == m_selectedTileForRules) {
            tile = t;
            break;
        }
    }

    if (tile) {
        rlutil::locate(6, line);
        std::cout << "For tile: ";
        SetConsoleTextAttribute(hConsole, tile->GetColor());
        std::cout << m_selectedTileForRules;
        SetConsoleTextAttribute(hConsole, 7);
        std::cout << " - " << tile->GetName();
    }

    line += 2;

    rlutil::locate(6, line);
    std::cout << ruleTypeName << ": ";

    if (m_editingRule) {
        SetConsoleTextAttribute(hConsole, 11);
        std::cout << m_tempRuleInput << "_";
    }
    else {
        if (currentRule && !currentRule->empty()) {
            std::cout << *currentRule;
        }
        else {
            std::cout << "(press ENTER to edit)";
        }
    }

    SetConsoleTextAttribute(hConsole, 7);
}

void WorldEditor::RenderFoodTab() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    if (m_needFullRedraw) {
        for (int i = 5; i < 22; i++) {
            ClearLine(i);
        }

        rlutil::locate(2, 5);
        SetConsoleTextAttribute(hConsole, 14);
        std::cout << "Food Configuration";
        SetConsoleTextAttribute(hConsole, 7);

        rlutil::locate(2, 6);
        std::cout << "------------------------------------------------------------";

        rlutil::locate(0, 8);
    }

    if (m_needFullRedraw || m_foodState != m_prevFoodState) {
        for (int i = 8; i < 17; i++) {
            ClearLine(i);
        }
        m_prevFoodState = m_foodState;

        rlutil::locate(0, 8);
    }

    switch (m_foodState) {
    case FoodState::MAIN_LIST:
        RenderFoodList(8);
        RenderBottomButtons();
        break;
    case FoodState::FOOD_ACTIONS:
        RenderFoodActions(8);
        break;
    case FoodState::EDITING_FOOD:
        RenderFoodEditing(8, false);
        break;
    case FoodState::ADDING_FOOD:
        RenderFoodEditing(8, true);
        break;
    }
}

void WorldEditor::RenderFoodList(int startLine) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    int line = startLine;

    rlutil::locate(2, line);
    std::cout << "  Available Food:";
    line++;

    const auto& allFoods = m_foodManager->GetAllFood();

    if (allFoods.empty()) {
        rlutil::locate(4, line);
        std::cout << "  No food items available. Add your first food to get started.";
        line += 2;
    }
    else {
        for (size_t i = 0; i < allFoods.size(); ++i) {
            const Food* food = allFoods[i];

            if (!food) continue;

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

            SetConsoleTextAttribute(hConsole, food->GetColor());
            std::cout << food->GetSymbol();

            SetConsoleTextAttribute(hConsole, isSelected ? 10 : 7);
            std::cout << " - " << food->GetName();

            SetConsoleTextAttribute(hConsole, 7);
            line++;
        }
    }

    bool addSelected = (m_selectedField == static_cast<int>(allFoods.size()) && m_selectedButton == 0);
    rlutil::locate(4, line);

    if (addSelected) {
        SetConsoleTextAttribute(hConsole, 10);
        std::cout << "> ";
    }
    else {
        SetConsoleTextAttribute(hConsole, 7);
        std::cout << "  ";
    }
    std::cout << "+ Add New Food";
    SetConsoleTextAttribute(hConsole, 7);
}

void WorldEditor::RenderFoodActions(int startLine) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    int line = startLine;

    const auto& allFoods = m_foodManager->GetAllFood();
    if (m_selectedFoodIndex >= 0 && m_selectedFoodIndex < static_cast<int>(allFoods.size())) {
        const Food* food = allFoods[m_selectedFoodIndex];

        if (food) {
            rlutil::locate(4, line);
            SetConsoleTextAttribute(hConsole, food->GetColor());
            std::cout << food->GetSymbol();
            SetConsoleTextAttribute(hConsole, 7);
            std::cout << " - " << food->GetName();
            line += 2;
        }
    }

    bool editSelected = (m_foodActionIndex == 0);
    bool deleteSelected = (m_foodActionIndex == 1);
    bool backSelected = (m_foodActionIndex == 2);

    rlutil::locate(6, line);
    if (editSelected) {
        SetConsoleTextAttribute(hConsole, 10);
        std::cout << "> ";
    }
    else {
        SetConsoleTextAttribute(hConsole, 7);
        std::cout << "  ";
    }
    std::cout << "- Edit";
    line++;

    rlutil::locate(6, line);
    if (deleteSelected) {
        SetConsoleTextAttribute(hConsole, 10);
        std::cout << "> ";
    }
    else {
        SetConsoleTextAttribute(hConsole, 7);
        std::cout << "  ";
    }
    std::cout << "- Delete";
    line++;

    rlutil::locate(6, line);
    if (backSelected) {
        SetConsoleTextAttribute(hConsole, 10);
        std::cout << "> ";
    }
    else {
        SetConsoleTextAttribute(hConsole, 7);
        std::cout << "  ";
    }
    std::cout << "- Back";

    SetConsoleTextAttribute(hConsole, 7);
}

void WorldEditor::RenderFoodEditing(int startLine, bool isNewFood) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    int line = startLine;

    if (m_needFullRedraw) {
        rlutil::locate(4, line);
        SetConsoleTextAttribute(hConsole, 14);
        if (isNewFood) {
            std::cout << "Add New Food";
        }
        else {
            std::cout << "Edit Food";
        }
        SetConsoleTextAttribute(hConsole, 7);
    }
    line += 2;

    std::vector<std::string> leftFields = {
        "Name: ",
        "Symbol: ",
        "Color: ",
        "Hunger Restore: ",
        "HP Restore: "
    };

    std::vector<std::string> rightFields = {
        "Spawn Weight: ",
        "Experience: "
    };

    for (int i = 0; i < leftFields.size(); ++i) {
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

        std::cout << leftFields[i];

        if (isEditing) {
            SetConsoleTextAttribute(hConsole, 11);
            std::cout << m_tempTileStringInput << "_";
        }
        else {
            SetConsoleTextAttribute(hConsole, 7);

            if (isNewFood) {
                switch (i) {
                case 0: std::cout << m_newFoodName; break;
                case 1:
                    std::cout << "'";
                    SetConsoleTextAttribute(hConsole, m_newFoodColor);
                    std::cout << m_newFoodSymbol;
                    SetConsoleTextAttribute(hConsole, 7);
                    std::cout << "'";
                    break;
                case 2: std::cout << m_newFoodColor; break;
                case 3: std::cout << m_newHungerRestore; break;
                case 4: std::cout << m_newHpRestore; break;
                }
            }
            else {
                switch (i) {
                case 0: std::cout << m_editedFoodName; break;
                case 1:
                    std::cout << "'";
                    SetConsoleTextAttribute(hConsole, m_editedFoodColor);
                    std::cout << m_editedFoodSymbol;
                    SetConsoleTextAttribute(hConsole, 7);
                    std::cout << "'";
                    break;
                case 2: std::cout << m_editedFoodColor; break;
                case 3: std::cout << m_editedHungerRestore; break;
                case 4: std::cout << m_editedHpRestore; break;
                }
            }
        }
    }

    for (int i = 0; i < rightFields.size(); ++i) {
        int fieldIndex = i + 5;
        bool isSelected = (fieldIndex == m_selectedField);
        bool isEditing = (m_isEditingText && m_editingTileFieldIndex == fieldIndex);

        rlutil::locate(40, line + 3 + i);

        if (isSelected) {
            SetConsoleTextAttribute(hConsole, 10);
            std::cout << "> ";
        }
        else {
            SetConsoleTextAttribute(hConsole, 7);
            std::cout << "  ";
        }

        std::cout << rightFields[i];

        if (isEditing) {
            SetConsoleTextAttribute(hConsole, 11);
            std::cout << m_tempTileStringInput << "_";
        }
        else {
            SetConsoleTextAttribute(hConsole, 7);

            if (isNewFood) {
                switch (fieldIndex) {
                case 5: std::cout << m_newSpawnWeight; break;
                case 6: std::cout << m_newExperience; break;
                }
            }
            else {
                switch (fieldIndex) {
                case 5: std::cout << m_editedSpawnWeight; break;
                case 6: std::cout << m_editedExperience; break;
                }
            }
        }
    }

    int buttonsStart = line + 6;

    if (m_needFullRedraw) {
        ClearLine(buttonsStart);
        ClearLine(buttonsStart + 1);
    }

    bool saveSelected = (m_selectedField == 7);
    RenderMenuItem(buttonsStart, "Save", saveSelected);

    bool cancelSelected = (m_selectedField == 8);
    RenderMenuItem(buttonsStart + 1, "Cancel", cancelSelected);

    SetConsoleTextAttribute(hConsole, 7);
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
        bool shiftPressed = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;

        if (shiftPressed) {
            SelectPreviousTab();
        }
        else {
            SelectNextTab();
        }
        m_needFullRedraw = true;
        Logger::Log("Tab pressed, needFullRedraw установлен в: " + std::to_string(m_needFullRedraw));
        return;
    }

    if (m_currentTab == EditorTab::CELLULAR_AUTOMATON) {
        HandleCellularInput();
        return;
    }

    if (m_isEditingText) {
        if (m_editingTileField) {
            if (m_currentTab == EditorTab::TILES) {
                HandleTileEditInput();
            }
            else if (m_currentTab == EditorTab::FOOD) {
                HandleFoodEditInput();
            }
            return;
        }
        HandleTextInput();
        return;
    }

    if (m_inputManager->IsMenuBack()) {
        if (m_currentTab == EditorTab::FOOD && m_foodState == FoodState::MAIN_LIST && m_selectedButton == 0) {
            if (m_editorMode == EditorMode::CREATE_TEMPLATE && !m_shouldCreate) {
                Logger::Log("Canceling template editing - checking if we should clean up...");

                if (IsNewTemplate()) {
                    Logger::Log("Cleaning up new template files...");
                    ClearTemplateFiles();
                }
                else {
                    Logger::Log("Editing existing template, no cleanup needed");
                }
                ResetTemplateData();
            }
            m_shouldReturn = true;
        }
        else if (m_currentTab == EditorTab::FOOD && m_foodState != FoodState::MAIN_LIST) {
            if (m_foodState == FoodState::ADDING_FOOD || m_foodState == FoodState::EDITING_FOOD) {
                m_newFoodName = "new_food";
                m_newFoodSymbol = '*';
                m_newFoodColor = 10;
                m_newHungerRestore = 10;
                m_newHpRestore = 5;
                m_newSpawnWeight = 1;
                m_newExperience = 5;

                m_editedFoodName = "";
                m_editedFoodSymbol = ' ';
                m_editedFoodColor = 10;
                m_editedHungerRestore = 0;
                m_editedHpRestore = 0;
                m_editedSpawnWeight = 1;
                m_editedExperience = 0;
            }
            m_foodState = FoodState::MAIN_LIST;
            m_selectedField = 0;
            m_selectedButton = 0;
            m_needFullRedraw = true;
        }
        else if (m_currentTab == EditorTab::TILES && m_tilesState == TilesState::MAIN_LIST && m_selectedButton == 0) {
            if (m_editorMode == EditorMode::CREATE_TEMPLATE && !m_shouldCreate) {
                Logger::Log("Canceling template editing - checking if we should clean up...");

                if (IsNewTemplate()) {
                    Logger::Log("Cleaning up new template files...");
                    ClearTemplateFiles();
                }
                else {
                    Logger::Log("Editing existing template, no cleanup needed");
                }
                ResetTemplateData();
            }
            m_shouldReturn = true;
        }
        else if (m_currentTab == EditorTab::TILES && m_tilesState != TilesState::MAIN_LIST) {
            if (m_tilesState == TilesState::ADDING_TILE || m_tilesState == TilesState::EDITING_TILE) {
                m_newTileName = "new_tile";
                m_newTileSymbol = 'A';
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
            m_tilesState = TilesState::MAIN_LIST;
            m_selectedField = 0;
            m_selectedButton = 0;
            m_needFullRedraw = true;
        }
        else if (m_currentTab == EditorTab::FOOD && m_foodState == FoodState::FOOD_ACTIONS) {
            m_foodState = FoodState::MAIN_LIST;
            m_needFullRedraw = true;
        }
        else if (m_currentTab == EditorTab::TILES && m_tilesState == TilesState::TILE_ACTIONS) {
            m_tilesState = TilesState::MAIN_LIST;
            m_needFullRedraw = true;
        }
        else {
            if (m_editorMode == EditorMode::CREATE_TEMPLATE && !m_shouldCreate) {
                Logger::Log("Canceling template editing - checking if we should clean up...");

                if (IsNewTemplate()) {
                    Logger::Log("Cleaning up new template files...");
                    ClearTemplateFiles();
                }
                else {
                    Logger::Log("Editing existing template, no cleanup needed");
                }
                ResetTemplateData();
            }
            m_shouldReturn = true;
        }
        return;
    }

    switch (m_currentTab) {
    case EditorTab::TILES:
        HandleTileInput();
        break;
    case EditorTab::FOOD:
        HandleFoodInput();
        break;
    case EditorTab::WORLD:
    case EditorTab::PLAYER:
    case EditorTab::CELLULAR_AUTOMATON:
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
                m_config.SetRandomGeneration(!m_config.GetRandomGeneration());
                m_isEditingText = false;
                m_editingField = -1;
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
            case 0: HandleTextInputGeneral(); break;
            case 1: HandleNumericInput(); break;
            case 2: HandleNumericInput(); break;
            case 3: HandleBooleanInput(); break;
            case 4: HandleNumericInput(); break;
            case 5: HandleFrequencyInput(); break;
            case 6: HandleNeighborRadiusInput(); break;
            }
        }
        break;
    }

    case EditorTab::PLAYER:
        HandleNumericInput();
        break;

    default:
        HandleTextInputGeneral();
        break;
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
    case EditorTab::FOOD:
        if (m_foodState == FoodState::MAIN_LIST) {
            const auto& allFoods = m_foodManager->GetAllFood();
            if (allFoods.empty()) {
                return 1;
            }
            return static_cast<int>(allFoods.size()) + 1;
        }
        else if (m_foodState == FoodState::FOOD_ACTIONS) {
            return 3;
        }
        else if (m_foodState == FoodState::EDITING_FOOD || m_foodState == FoodState::ADDING_FOOD) {
            return 7;
        }
        return 0;
    case EditorTab::CELLULAR_AUTOMATON:
    {
        int tileCount = 0;
        for (int tileId : m_availableTileIds) {
            TileType* tile = m_tileManager->GetTileType(tileId);
            if (tile && tile->GetCharacter() != ' ') {
                tileCount++;
            }
        }

        if (tileCount == 0) {
            return 1;
        }

        return tileCount * 3;
    }
    default:
        return 0;
    }
}

void WorldEditor::ClearDefaultTiles() {
    m_availableTileIds.clear();
}

void WorldEditor::SelectNextTab() {
    int current = static_cast<int>(m_currentTab);
    current = (current + 1) % 5;
    m_currentTab = static_cast<EditorTab>(current);

    if (m_currentTab == EditorTab::CELLULAR_AUTOMATON) {
        m_cellularState = CellularAutomatonState::MAIN_LIST;
        m_selectedRuleType = 0;
        m_editingRule = false;
        m_selectedTileForRules = '\0';
        m_tempRuleInput = "";
        m_selectedField = 0;
        m_cellularScrollOffset = 0;
    }

    if (m_currentTab == EditorTab::TILES) {
        m_tilesState = TilesState::MAIN_LIST;
        m_selectedTileIndex = 0;
        m_tileActionIndex = 0;
    }

    if (m_currentTab == EditorTab::FOOD) {
        m_foodState = FoodState::MAIN_LIST;
        m_selectedFoodIndex = 0;
        m_foodActionIndex = 0;
    }

    m_selectedField = 0;
    m_selectedButton = 0;
    m_isEditingText = false;
    m_editingField = -1;
    m_editingTileField = false;
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
    }
}

void WorldEditor::ConfirmSelection() {
    if (m_currentTab == EditorTab::TILES && m_tilesState == TilesState::MAIN_LIST) {
        if (m_selectedButton == 0) {
            if (m_availableTileIds.empty()) {
                if (m_selectedField == 0) {
                    Logger::Log("Starting to add new tile...");
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
                    Logger::Log("Starting to add new tile...");
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
    else if (m_currentTab == EditorTab::FOOD && m_foodState == FoodState::MAIN_LIST) {
        if (m_selectedButton == 0) {
            const auto& allFoods = m_foodManager->GetAllFood();
            if (allFoods.empty()) {
                if (m_selectedField == 0) {
                    Logger::Log("Starting to add new food...");
                    StartAddingFood();
                }
            }
            else {
                if (m_selectedField < static_cast<int>(allFoods.size())) {
                    m_selectedFoodIndex = m_selectedField;
                    m_foodActionIndex = 0;
                    m_foodState = FoodState::FOOD_ACTIONS;
                    m_needFullRedraw = true;
                    Logger::Log("Selected food at index " + std::to_string(m_selectedField));
                }
                else if (m_selectedField == static_cast<int>(allFoods.size())) {
                    Logger::Log("Starting to add new food (from list)...");
                    StartAddingFood();
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
    else if (m_currentTab == EditorTab::FOOD && m_foodState == FoodState::FOOD_ACTIONS) {
        const auto& allFoods = m_foodManager->GetAllFood();
        switch (m_foodActionIndex) {
        case 0: // Edit
            if (m_selectedFoodIndex >= 0 && m_selectedFoodIndex < static_cast<int>(allFoods.size())) {
                StartEditingFood();
            }
            break;
        case 1: // Delete
        {
            if (m_selectedFoodIndex >= 0 && m_selectedFoodIndex < static_cast<int>(allFoods.size())) {
                DeleteSelectedFood();
                m_foodState = FoodState::MAIN_LIST;
                const auto& newFoods = m_foodManager->GetAllFood();
                m_selectedField = min(m_selectedField, static_cast<int>(newFoods.size() - 1));
                if (m_selectedField < 0) m_selectedField = 0;
                m_needFullRedraw = true;
            }
            break;
        }
        case 2: // Back
            m_foodState = FoodState::MAIN_LIST;
            m_needFullRedraw = true;
            break;
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

    if (m_tileManager && m_tileManager->GetAllTiles().size() <= 2) {
        Logger::Log("WARNING: No user-defined tiles found!");
        Logger::Log("World will be created with only system tiles (air/border).");
        Logger::Log("Player can add tiles later in the Tiles tab.");
    }

    SaveWorldConfiguration();
    SaveCellularAutomatonRules();

    if (m_editorMode == EditorMode::CREATE_WORLD) {
        if (!m_saveSystem) {
            m_saveSystem = std::make_unique<SaveSystem>();
        }

        std::string savePath = m_saveSystem->GetSaveSlotPath(m_slot);
        bool allSaved = SaveAllConfigurations(savePath);

        if (allSaved) {
            std::ofstream infoFile(savePath + "/save_info.txt");
            if (infoFile.is_open()) {
                infoFile << m_config.GetWorldName() << "\n";
                infoFile << m_saveSystem->GetCurrentDateTime() << "\n";
                infoFile << m_saveSystem->GetCurrentDateTime() << "\n";
                infoFile.close();
            }

            Logger::Log("Successfully created world: " + m_config.GetWorldName());
            m_shouldCreate = true;
        }
        else {
            Logger::Log("ERROR: Failed to create world: " + m_config.GetWorldName());
        }
    }
    else {
        if (CreateTemplate(m_config.GetWorldName())) {
            Logger::Log("=== TEMPLATE CREATED SUCCESSFULLY ===");
            m_shouldCreate = true;
        }
        else {
            Logger::Log("=== TEMPLATE CREATION FAILED - CLEANING UP ===");
            ClearTemplateFiles();
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
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    int width = csbi.dwSize.X;

    std::string spaces(width, ' ');
    std::cout << spaces;
    rlutil::locate(0, line);
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
        float newValue = max(0.0f, m_config.GetNoiseFrequency() - 0.01f);
        m_config.SetNoiseFrequency(newValue);
        m_tempStringInput = std::to_string(m_config.GetNoiseFrequency());
        m_needFullRedraw = true;
        return;
    }
    else if (m_inputManager->IsKeyPressed(VK_RIGHT) || m_inputManager->IsKeyPressed('D')) {
        float newValue = min(1.0f, m_config.GetNoiseFrequency() + 0.01f);
        m_config.SetNoiseFrequency(newValue);
        m_tempStringInput = std::to_string(m_config.GetNoiseFrequency());
        m_needFullRedraw = true;
        return;
    }

    for (char c = '0'; c <= '9'; c++) {
        if (m_inputManager->IsKeyPressed(c)) {
            m_tempStringInput += c;
            m_needFullRedraw = true;
            return;
        }
    }

    if (m_inputManager->IsKeyPressed('.')) {
        if (m_tempStringInput.find('.') == std::string::npos) {
            m_tempStringInput += '.';
            m_needFullRedraw = true;
        }
        return;
    }

    if (m_inputManager->IsKeyPressed(VK_BACK) && !m_tempStringInput.empty()) {
        m_tempStringInput.pop_back();
        m_needFullRedraw = true;
        return;
    }
}

void WorldEditor::HandleNeighborRadiusInput() {
    if (m_inputManager->IsKeyPressed(VK_LEFT) || m_inputManager->IsKeyPressed('A')) {
        int newValue = max(0, m_config.GetNeighborRadius() - 1);
        m_config.SetNeighborRadius(newValue);
        m_tempStringInput = std::to_string(m_config.GetNeighborRadius());
        m_needFullRedraw = true;
        return;
    }
    else if (m_inputManager->IsKeyPressed(VK_RIGHT) || m_inputManager->IsKeyPressed('D')) {
        int newValue = min(10, m_config.GetNeighborRadius() + 1);
        m_config.SetNeighborRadius(newValue);
        m_tempStringInput = std::to_string(m_config.GetNeighborRadius());
        m_needFullRedraw = true;
        return;
    }

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

    int userTileCount = 0;
    if (m_tileManager) {
        const auto& allTiles = m_tileManager->GetAllTiles();
        for (const auto& pair : allTiles) {
            int tileId = pair.first;
            if (tileId != 0 && tileId != -1 && tileId != 2) {
                userTileCount++;
            }
        }
    }

    if (userTileCount == 0) {
        Logger::Log("NOTE: Template has no user-defined tiles.");
        Logger::Log("System tiles (air/border) are hidden from user view.");
    }

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

        SaveCellularAutomatonRules();

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

    if (!SaveCellularAutomatonConfigPreserve(directory)) {
        Logger::Log("WARNING: Failed to save cellular automaton config (may preserve existing)");
    }

    success = SaveFoodConfig(directory) && success;

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

    Logger::Log("=== SAVING CELLULAR AUTOMATON RULES ===");
    Logger::Log("Saving to: " + automatonConfigPath);

    if (fs::exists(automatonConfigPath)) {
        Logger::Log("Loading existing cellular automaton config to preserve rules...");
        return true;
    }

    std::ofstream file(automatonConfigPath);
    if (!file.is_open()) {
        Logger::Log("ERROR: Cannot open cellular automaton config for writing: " + automatonConfigPath);
        return false;
    }
    std::set<char> allTiles;
    for (const auto& pair : m_survivalRules) allTiles.insert(pair.first);
    for (const auto& pair : m_birthRules) allTiles.insert(pair.first);
    for (const auto& pair : m_deathRules) allTiles.insert(pair.first);

    Logger::Log("Saving rules for " + std::to_string(allTiles.size()) + " tiles");

    for (char tileChar : allTiles) {
        file << tileChar << "\n";

        auto survivalIt = m_survivalRules.find(tileChar);
        auto birthIt = m_birthRules.find(tileChar);
        auto deathIt = m_deathRules.find(tileChar);

        bool hasAnyRule = false;

        if (survivalIt != m_survivalRules.end() && !survivalIt->second.empty()) {
            file << "survival=" << survivalIt->second << "\n";
            hasAnyRule = true;
            Logger::Log("  Tile '" + std::string(1, tileChar) + "' survival: " + survivalIt->second);
        }

        if (birthIt != m_birthRules.end() && !birthIt->second.empty()) {
            file << "birth=" << birthIt->second << "\n";
            hasAnyRule = true;
            Logger::Log("  Tile '" + std::string(1, tileChar) + "' birth: " + birthIt->second);
        }

        if (deathIt != m_deathRules.end() && !deathIt->second.empty()) {
            file << "death=" << deathIt->second << "\n";
            hasAnyRule = true;
            Logger::Log("  Tile '" + std::string(1, tileChar) + "' death: " + deathIt->second);
        }

        if (hasAnyRule) {
            file << "\n";
        }
    }

    file.close();
    Logger::Log("Saved cellular automaton rules to: " + automatonConfigPath);
    return true;
}

bool WorldEditor::SaveFoodConfig(const std::string& directory) {
    std::string foodConfigPath = directory + "/food.cfg";
    std::ofstream destFile(foodConfigPath);

    if (!destFile.is_open()) {
        Logger::Log("ERROR: Cannot open food config file: " + foodConfigPath);
        return false;
    }

    const auto& allFoods = m_foodManager->GetAllFood();

    destFile << "# Food configuration\n";
    destFile << "# Format: ID Name Symbol Color HungerRestore HpRestore SpawnWeight Experience\n";

    if (allFoods.empty()) {
        Logger::Log("No food items found, saving empty config");
    }
    else {
        Logger::Log("Saving " + std::to_string(allFoods.size()) + " food items");
        for (size_t i = 0; i < allFoods.size(); ++i) {
            const Food* food = allFoods[i];
            if (food) {
                destFile << i << " "
                    << food->GetName() << " "
                    << food->GetSymbol() << " "
                    << food->GetColor() << " "
                    << food->GetHungerRestore() << " "
                    << food->GetHpRestore() << " "
                    << food->GetSpawnWeight() << " "
                    << food->GetExperience() << "\n";

                Logger::Log("  - " + food->GetName() + " (ID: " + std::to_string(i) + ")");
            }
        }
    }

    destFile.close();
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

    if (m_editorMode == EditorMode::CREATE_WORLD) {
        m_foodConfigPath = "saves/slot" + std::to_string(m_slot) + "/food.cfg";
    }

    Logger::Log("Loaded template configuration: " + config.GetWorldName() +
        ", food config path: " + m_foodConfigPath);
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

void WorldEditor::LoadAvailableFood() {
    Logger::Log("=== LOADING AVAILABLE FOOD ===");

    if (!m_foodManager) {
        Logger::Log("ERROR: No food manager");
        return;
    }

    const auto& allFoods = m_foodManager->GetAllFood();
    Logger::Log("Food manager has " + std::to_string(allFoods.size()) + " food items");

    for (size_t i = 0; i < allFoods.size(); ++i) {
        if (const Food* food = allFoods[i]) {
            Logger::Log("Found food [" + std::to_string(i) + "]: " +
                food->GetName() + " ('" + std::string(1, food->GetSymbol()) + "')");
        }
    }

    Logger::Log("Loaded " + std::to_string(allFoods.size()) + " food items");
}

void WorldEditor::HandleFoodInput() {
    if (m_foodState == FoodState::EDITING_FOOD || m_foodState == FoodState::ADDING_FOOD) {
        if (m_isEditingText && m_editingTileField) {
            HandleFoodEditInput();
            return;
        }
        HandleFoodEditNavigation();
        return;
    }

    if (m_foodState == FoodState::FOOD_ACTIONS) {
        HandleFoodActionsNavigation();
        return;
    }

    HandleStandardInput();
}

void WorldEditor::HandleFoodActionsNavigation() {
    if (m_inputManager->IsMenuUp()) {
        m_foodActionIndex = (m_foodActionIndex - 1 + 3) % 3;
        m_needFullRedraw = true;
    }
    else if (m_inputManager->IsMenuDown()) {
        m_foodActionIndex = (m_foodActionIndex + 1) % 3;
        m_needFullRedraw = true;
    }
    else if (m_inputManager->IsMenuSelect()) {
        const auto& allFoods = m_foodManager->GetAllFood();
        switch (m_foodActionIndex) {
        case 0: // Edit
            if (m_selectedFoodIndex >= 0 && m_selectedFoodIndex < static_cast<int>(allFoods.size())) {
                StartEditingFood();
            }
            break;
        case 1: // Delete
            if (m_selectedFoodIndex >= 0 && m_selectedFoodIndex < static_cast<int>(allFoods.size())) {
                DeleteSelectedFood();
                m_foodState = FoodState::MAIN_LIST;
                m_selectedField = min(m_selectedField, static_cast<int>(allFoods.size() - 1));
                if (m_selectedField < 0) m_selectedField = 0;
                m_needFullRedraw = true;
            }
            break;
        case 2: // Back
            m_foodState = FoodState::MAIN_LIST;
            m_needFullRedraw = true;
            break;
        }
    }
    else if (m_inputManager->IsMenuBack()) {
        m_foodState = FoodState::MAIN_LIST;
        m_needFullRedraw = true;
    }
}

void WorldEditor::StartEditingFood() {
    const auto& allFoods = m_foodManager->GetAllFood();
    if (m_selectedFoodIndex < 0 || m_selectedFoodIndex >= static_cast<int>(allFoods.size())) {
        m_foodState = FoodState::MAIN_LIST;
        m_needFullRedraw = true;
        return;
    }

    const Food* food = allFoods[m_selectedFoodIndex];

    if (!food) {
        m_foodState = FoodState::MAIN_LIST;
        m_needFullRedraw = true;
        return;
    }

    m_editedFoodName = food->GetName();
    m_editedFoodSymbol = food->GetSymbol();
    m_editedFoodColor = food->GetColor();
    m_editedHungerRestore = food->GetHungerRestore();
    m_editedHpRestore = food->GetHpRestore();
    m_editedSpawnWeight = food->GetSpawnWeight();
    m_editedExperience = food->GetExperience();

    m_foodState = FoodState::EDITING_FOOD;
    m_selectedField = 0;
    m_editingTileField = false;
    m_needFullRedraw = true;
}

void WorldEditor::HandleFoodEditNavigation() {
    int fieldCount = 7; // 7 полей (Name, Symbol, Color, HungerRestore, HpRestore, SpawnWeight, Experience)
    int buttonCount = 2; // Save, Cancel
    int totalCount = fieldCount + buttonCount;

    // Матрица навигации: [столбец][строка]
    // 0: Name (0,0)
    // 1: Symbol (0,1)
    // 2: Color (0,2)
    // 3: Hunger Restore (0,3)
    // 4: HP Restore (0,4)
    // 5: Spawn Weight (1,3)
    // 6: Experience (1,4)

    if (m_inputManager->IsMenuUp()) {
        // Перемещение вверх
        switch (m_selectedField) {
        case 0: // Name -> остаемся на Name
            break;
        case 1: // Symbol -> Name
            m_selectedField = 0;
            break;
        case 2: // Color -> Symbol
            m_selectedField = 1;
            break;
        case 3: // Hunger Restore -> Color
            m_selectedField = 2;
            break;
        case 4: // HP Restore -> Hunger Restore
            m_selectedField = 3;
            break;
        case 5: // Spawn Weight -> Hunger Restore
            m_selectedField = 3;
            break;
        case 6: // Experience -> HP Restore или Spawn Weight
            if (m_selectedField == 4) m_selectedField = 4;
            else m_selectedField = 5;
            break;
        case 7: // Save -> HP Restore
            m_selectedField = 4;
            break;
        case 8: // Cancel -> Experience
            m_selectedField = 6;
            break;
        }
    }
    else if (m_inputManager->IsMenuDown()) {
        // Перемещение вниз
        switch (m_selectedField) {
        case 0: // Name -> Symbol
            m_selectedField = 1;
            break;
        case 1: // Symbol -> Color
            m_selectedField = 2;
            break;
        case 2: // Color -> Hunger Restore
            m_selectedField = 3;
            break;
        case 3: // Hunger Restore -> HP Restore или Spawn Weight
            // По умолчанию переходим на HP Restore
            m_selectedField = 4;
            break;
        case 4: // HP Restore -> Save
            m_selectedField = 7;
            break;
        case 5: // Spawn Weight -> Experience
            m_selectedField = 6;
            break;
        case 6: // Experience -> Cancel
            m_selectedField = 8;
            break;
        case 7: // Save -> Cancel
            m_selectedField = 8;
            break;
        case 8: // Cancel -> Save
            m_selectedField = 7;
            break;
        }
    }
    else if (m_inputManager->IsKeyPressed(VK_LEFT) || m_inputManager->IsKeyPressed('A')) {
        // Перемещение влево между столбцами
        switch (m_selectedField) {
        case 5: // Spawn Weight -> Hunger Restore
            m_selectedField = 3;
            break;
        case 6: // Experience -> HP Restore
            m_selectedField = 4;
            break;
        case 7: // Save -> HP Restore (если нужно)
            m_selectedField = 4;
            break;
        case 8: // Cancel -> Experience
            m_selectedField = 6;
            break;
        }
    }
    else if (m_inputManager->IsKeyPressed(VK_RIGHT) || m_inputManager->IsKeyPressed('D')) {
        // Перемещение вправо между столбцами
        switch (m_selectedField) {
        case 3: // Hunger Restore -> Spawn Weight
            m_selectedField = 5;
            break;
        case 4: // HP Restore -> Experience
            m_selectedField = 6;
            break;
        }
    }
    else if (m_inputManager->IsMenuSelect()) {
        if (m_selectedField < fieldCount) {
            StartEditingFoodField();
        }
        else if (m_selectedField == fieldCount) { // Save
            if (m_foodState == FoodState::ADDING_FOOD) {
                AddNewFood();
                m_foodState = FoodState::MAIN_LIST;
                m_selectedField = 0;
                m_selectedButton = 0;
            }
            else {
                ApplyFoodEdit();
                m_foodState = FoodState::MAIN_LIST;
                m_selectedField = m_selectedFoodIndex;
                m_selectedButton = 0;
            }
            m_needFullRedraw = true;
        }
        else if (m_selectedField == fieldCount + 1) { // Cancel
            if (m_foodState == FoodState::ADDING_FOOD) {
                m_newFoodName = "new_food";
                m_newFoodSymbol = '*';
                m_newFoodColor = 10;
                m_newHungerRestore = 10;
                m_newHpRestore = 5;
                m_newSpawnWeight = 1;
                m_newExperience = 5;
            }
            m_foodState = FoodState::MAIN_LIST;
            m_selectedField = (m_foodState == FoodState::MAIN_LIST && m_availableFoodIds.empty()) ? 0 :
                (m_selectedFoodIndex < static_cast<int>(m_availableFoodIds.size()) ? m_selectedFoodIndex : 0);
            m_selectedButton = 0;
            m_needFullRedraw = true;
        }
    }
    else if (m_inputManager->IsMenuBack()) {
        if (m_foodState == FoodState::ADDING_FOOD) {
            m_newFoodName = "new_food";
            m_newFoodSymbol = '*';
            m_newFoodColor = 10;
            m_newHungerRestore = 10;
            m_newHpRestore = 5;
            m_newSpawnWeight = 1;
            m_newExperience = 5;
        }
        m_foodState = FoodState::MAIN_LIST;
        m_selectedField = (m_foodState == FoodState::MAIN_LIST && m_availableFoodIds.empty()) ? 0 :
            (m_selectedFoodIndex < static_cast<int>(m_availableFoodIds.size()) ? m_selectedFoodIndex : 0);
        m_selectedButton = 0;
        m_needFullRedraw = true;
    }
}

void WorldEditor::StartAddingFood() {
    m_newFoodName = "new_food";
    m_newFoodSymbol = '*';
    m_newFoodColor = 10;
    m_newHungerRestore = 10;
    m_newHpRestore = 5;
    m_newSpawnWeight = 1;
    m_newExperience = 5;

    m_foodState = FoodState::ADDING_FOOD;
    m_selectedField = 0;
    m_editingTileField = false;
    m_needFullRedraw = true;
}

void WorldEditor::StartEditingFoodField() {
    m_isEditingText = true;
    m_editingTileField = true;
    m_editingTileFieldIndex = m_selectedField;

    m_tempTileStringInput = "";

    if (m_foodState == FoodState::EDITING_FOOD) {
        const auto& allFoods = m_foodManager->GetAllFood();
        if (m_selectedFoodIndex >= 0 && m_selectedFoodIndex < static_cast<int>(allFoods.size())) {
            const Food* food = allFoods[m_selectedFoodIndex];

            if (food) {
                switch (m_editingTileFieldIndex) {
                case 0: // Name
                    m_tempTileStringInput = food->GetName();
                    break;
                case 1: // Symbol
                    m_tempTileStringInput = std::string(1, food->GetSymbol());
                    break;
                case 2: // Color
                    m_tempTileStringInput = std::to_string(food->GetColor());
                    break;
                case 3: // Hunger Restore
                    m_tempTileStringInput = std::to_string(food->GetHungerRestore());
                    break;
                case 4: // HP Restore
                    m_tempTileStringInput = std::to_string(food->GetHpRestore());
                    break;
                case 5: // Spawn Weight
                    m_tempTileStringInput = std::to_string(food->GetSpawnWeight());
                    break;
                case 6: // Experience
                    m_tempTileStringInput = std::to_string(food->GetExperience());
                    break;
                }
            }
        }
    }
    else if (m_foodState == FoodState::ADDING_FOOD) {
        switch (m_editingTileFieldIndex) {
        case 0: // Name
            m_tempTileStringInput = m_newFoodName;
            break;
        case 1: // Symbol
            m_tempTileStringInput = std::string(1, m_newFoodSymbol);
            break;
        case 2: // Color
            m_tempTileStringInput = std::to_string(m_newFoodColor);
            break;
        case 3: // Hunger Restore
            m_tempTileStringInput = std::to_string(m_newHungerRestore);
            break;
        case 4: // HP Restore
            m_tempTileStringInput = std::to_string(m_newHpRestore);
            break;
        case 5: // Spawn Weight
            m_tempTileStringInput = std::to_string(m_newSpawnWeight);
            break;
        case 6: // Experience
            m_tempTileStringInput = std::to_string(m_newExperience);
            break;
        }
    }

    m_needFullRedraw = true;
}

void WorldEditor::HandleFoodEditInput() {
    if (m_inputManager->IsKeyPressed(VK_RETURN) || m_inputManager->IsKeyPressed(VK_SPACE)) {
        if (m_foodState == FoodState::ADDING_FOOD) {
            SaveNewFoodField();
        }
        else {
            SaveEditedFoodField();
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

    switch (m_editingTileFieldIndex) {
    case 2: // Color
        HandleFoodColorInput();
        break;
    case 3: // Hunger Restore
    case 4: // HP Restore
    case 5: // Spawn Weight
    case 6: // Experience
        HandleFoodNumericInput();
        break;
    case 0: // Name
        HandleFoodNameInput();
        break;
    case 1: // Symbol
        HandleFoodSymbolInput();
        break;
    }
}

void WorldEditor::HandleFoodColorInput() {
    if (m_inputManager->IsKeyPressed(VK_LEFT) || m_inputManager->IsKeyPressed('A')) {
        if (m_tempTileStringInput.empty()) {
            m_tempTileStringInput = "10";
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
            m_tempTileStringInput = "10";
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

void WorldEditor::HandleFoodNumericInput() {
    for (char c = '0'; c <= '9'; c++) {
        if (m_inputManager->IsKeyPressed(c)) {
            if (m_tempTileStringInput == "0" && c == '0') {
                return;
            }
            else if (m_tempTileStringInput == "0" && c != '0') {
                m_tempTileStringInput = std::string(1, c);
            }
            else {
                m_tempTileStringInput += c;
            }
            m_needFullRedraw = true;
            return;
        }
    }

    if (m_inputManager->IsKeyPressed('-') && m_tempTileStringInput.empty()) {
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

void WorldEditor::HandleFoodNameInput() {
    if (IsCtrlPressed() && m_inputManager->IsKeyPressed('C')) {
        if (!m_tempTileStringInput.empty()) {
            CopyToClipboard(m_tempTileStringInput);
            Logger::Log("Copied food name to clipboard: " + m_tempTileStringInput);
        }
        return;
    }

    if (IsCtrlPressed() && m_inputManager->IsKeyPressed('V')) {
        std::string clipboardText = PasteFromClipboard();
        if (!clipboardText.empty()) {
            m_tempTileStringInput += clipboardText;
            Logger::Log("Pasted food name from clipboard: " + clipboardText);
            m_needFullRedraw = true;
        }
        return;
    }

    for (char c = '0'; c <= '9'; c++) {
        if (m_inputManager->IsKeyPressed(c) && !IsCtrlPressed()) {
            m_tempTileStringInput += c;
            m_needFullRedraw = true;
            return;
        }
    }

    for (char c = 'A'; c <= 'Z'; c++) {
        if (m_inputManager->IsKeyPressed(c) && !IsCtrlPressed()) {
            m_tempTileStringInput += c;
            m_needFullRedraw = true;
            return;
        }
    }

    for (char c = 'a'; c <= 'z'; c++) {
        if (m_inputManager->IsKeyPressed(c) && !IsCtrlPressed()) {
            m_tempTileStringInput += c;
            m_needFullRedraw = true;
            return;
        }
    }

    if (m_inputManager->IsKeyPressed('_') || m_inputManager->IsKeyPressed('-')) {
        m_tempTileStringInput += '_';
        m_needFullRedraw = true;
        return;
    }

    if (m_inputManager->IsKeyPressed(VK_BACK) && !m_tempTileStringInput.empty()) {
        m_tempTileStringInput.pop_back();
        m_needFullRedraw = true;
        return;
    }

    if (m_inputManager->IsKeyPressed(VK_DELETE)) {
        m_tempTileStringInput.clear();
        m_needFullRedraw = true;
        return;
    }
}

void WorldEditor::HandleFoodSymbolInput() {
    if (m_inputManager->IsKeyPressed(VK_RETURN) || m_inputManager->IsKeyPressed(VK_SPACE)) {
        if (m_foodState == FoodState::ADDING_FOOD) {
            SaveNewFoodField();
        }
        else {
            SaveEditedFoodField();
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

    if (m_inputManager->IsKeyPressed(VK_BACK) && !m_tempTileStringInput.empty()) {
        m_tempTileStringInput.pop_back();
        m_needFullRedraw = true;
        return;
    }

    if (m_inputManager->IsKeyPressed(VK_DELETE)) {
        m_tempTileStringInput.clear();
        m_needFullRedraw = true;
        return;
    }

    bool shiftPressed = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    bool capsLockOn = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;

    for (char c = '0'; c <= '9'; c++) {
        if (m_inputManager->IsKeyPressed(c)) {
            if (shiftPressed) {
                static const std::unordered_map<char, char> shiftDigits = {
                    {'0', ')'}, {'1', '!'}, {'2', '@'}, {'3', '#'}, {'4', '$'},
                    {'5', '%'}, {'6', '^'}, {'7', '&'}, {'8', '*'}, {'9', '('}
                };
                auto it = shiftDigits.find(c);
                if (it != shiftDigits.end()) {
                    m_tempTileStringInput = std::string(1, it->second);
                    m_needFullRedraw = true;
                }
            }
            else {
                m_tempTileStringInput = std::string(1, c);
                m_needFullRedraw = true;
            }
            return;
        }
    }

    for (int vkCode = 'A'; vkCode <= 'Z'; vkCode++) {
        if (m_inputManager->IsKeyPressed(vkCode)) {
            char c = static_cast<char>(vkCode);
            if (shiftPressed ^ capsLockOn) {
                m_tempTileStringInput = std::string(1, c);
            }
            else {
                m_tempTileStringInput = std::string(1, c + 32);
            }
            m_needFullRedraw = true;
            return;
        }
    }

    static const std::unordered_map<int, std::pair<char, char>> specialKeys = {
        {VK_OEM_MINUS, {'-', '_'}},
        {VK_OEM_PLUS, {'=', '+'}},
        {VK_OEM_1, {';', ':'}},
        {VK_OEM_2, {'/', '?'}},
        {VK_OEM_3, {'`', '~'}},
        {VK_OEM_4, {'[', '{'}},
        {VK_OEM_5, {'\\', '|'}},
        {VK_OEM_6, {']', '}'}},
        {VK_OEM_7, {'\'', '"'}},
        {VK_OEM_COMMA, {',', '<'}},
        {VK_OEM_PERIOD, {'.', '>'}},
        {VK_SPACE, {' ', ' '}},
    };

    for (const auto& [vkCode, chars] : specialKeys) {
        if (m_inputManager->IsKeyPressed(vkCode)) {
            char symbol = shiftPressed ? chars.second : chars.first;
            m_tempTileStringInput = std::string(1, symbol);
            m_needFullRedraw = true;
            return;
        }
    }

    for (int i = 32; i <= 255; i++) {
        if (m_inputManager->IsKeyPressed(i)) {
            if ((i >= '0' && i <= '9') || (i >= 'A' && i <= 'Z')) {
                continue;
            }

            bool isSpecialVK = false;
            for (const auto& [vkCode, _] : specialKeys) {
                if (vkCode == i) {
                    isSpecialVK = true;
                    break;
                }
            }
            if (isSpecialVK) {
                continue;
            }

            m_tempTileStringInput = std::string(1, static_cast<char>(i));
            m_needFullRedraw = true;
            return;
        }
    }
}

void WorldEditor::SaveNewFoodField() {
    if (m_tempTileStringInput.empty()) return;

    try {
        switch (m_editingTileFieldIndex) {
        case 0:
            m_newFoodName = m_tempTileStringInput;
            break;
        case 1:
            if (!m_tempTileStringInput.empty()) {
                m_newFoodSymbol = m_tempTileStringInput[0];
            }
            break;
        case 2:
        {
            int newColor = std::stoi(m_tempTileStringInput);
            m_newFoodColor = std::clamp(newColor, 0, 15);
        }
        break;
        case 3:
        {
            int value = std::stoi(m_tempTileStringInput);
            m_newHungerRestore = value;
        }
        break;
        case 4:
        {
            int value = std::stoi(m_tempTileStringInput);
            m_newHpRestore = value;
        }
        break;
        case 5:
        {
            int value = std::stoi(m_tempTileStringInput);
            m_newSpawnWeight = std::clamp(value, 1, 100);
        }
        break;
        case 6:
        {
            int value = std::stoi(m_tempTileStringInput);
            m_newExperience = value;
        }
        break;
        }
    }
    catch (const std::exception& e) {
        Logger::Log("ERROR: Invalid food input: " + m_tempTileStringInput);
    }
}

void WorldEditor::AddNewFood() {
    try {
        Logger::Log("=== ADDING NEW FOOD ===");

        std::ofstream foodFile(m_foodConfigPath, std::ios::app);
        if (!foodFile.is_open()) {
            Logger::Log("ERROR: Cannot open food config file: " + m_foodConfigPath);
            return;
        }

        int newId = m_availableFoodIds.empty() ? 0 : (*std::max_element(m_availableFoodIds.begin(), m_availableFoodIds.end())) + 1;

        foodFile << newId << " "
            << m_newFoodName << " "
            << m_newFoodSymbol << " "
            << m_newFoodColor << " "
            << m_newHungerRestore << " "
            << m_newHpRestore << " "
            << m_newSpawnWeight << " "
            << m_newExperience << "\n";

        foodFile.close();

        m_foodManager->LoadFromFile(m_foodConfigPath);
        LoadAvailableFood();

        Logger::Log("Added new food: " + m_newFoodName +
            " (Hunger: " + std::to_string(m_newHungerRestore) +
            ", HP: " + std::to_string(m_newHpRestore) +
            ", XP: " + std::to_string(m_newExperience) + ")");

        m_newFoodName = "new_food_" + std::to_string(newId + 1);
        m_newFoodSymbol = static_cast<char>('*' + ((newId + 1) % 10));
        m_newFoodColor = (newId + 1) % 15 + 1;
        m_newHungerRestore = 10;
        m_newHpRestore = 5;
        m_newSpawnWeight = 1;
        m_newExperience = 5;

        m_foodState = FoodState::MAIN_LIST;
        m_selectedField = 0;
        m_selectedButton = 0;
        m_needFullRedraw = true;

        Logger::Log("=== NEW FOOD ADDED ===");

    }
    catch (const std::exception& e) {
        Logger::Log("ERROR: Failed to add new food: " + std::string(e.what()));
    }
}

void WorldEditor::ApplyFoodEdit() {
    try {
        Logger::Log("=== EDITING FOOD ===");

        const auto& allFoods = m_foodManager->GetAllFood();
        if (m_selectedFoodIndex < 0 || m_selectedFoodIndex >= static_cast<int>(allFoods.size())) {
            Logger::Log("ERROR: Invalid food index");
            return;
        }

        std::ofstream foodFile(m_foodConfigPath);
        if (!foodFile.is_open()) {
            Logger::Log("ERROR: Cannot open food config file for editing: " + m_foodConfigPath);
            return;
        }

        for (size_t i = 0; i < allFoods.size(); ++i) {
            if (i == static_cast<size_t>(m_selectedFoodIndex)) {
                foodFile << i << " "
                    << m_editedFoodName << " "
                    << m_editedFoodSymbol << " "
                    << m_editedFoodColor << " "
                    << m_editedHungerRestore << " "
                    << m_editedHpRestore << " "
                    << m_editedSpawnWeight << " "
                    << m_editedExperience << "\n";
            }
            else {
                const Food* food = allFoods[i];
                foodFile << i << " "
                    << food->GetName() << " "
                    << food->GetSymbol() << " "
                    << food->GetColor() << " "
                    << food->GetHungerRestore() << " "
                    << food->GetHpRestore() << " "
                    << food->GetSpawnWeight() << " "
                    << food->GetExperience() << "\n";
            }
        }

        foodFile.close();

        m_foodManager->LoadFromFile(m_foodConfigPath);
        LoadAvailableFood();

        Logger::Log("Food updated: " + m_editedFoodName);

    }
    catch (const std::exception& e) {
        Logger::Log("ERROR: Failed to edit food: " + std::string(e.what()));
    }
}

void WorldEditor::DeleteSelectedFood() {
    const auto& allFoods = m_foodManager->GetAllFood();
    if (allFoods.empty() || m_selectedFoodIndex < 0 || m_selectedFoodIndex >= static_cast<int>(allFoods.size())) {
        Logger::Log("ERROR: Cannot delete - invalid food index");
        return;
    }

    try {
        Logger::Log("=== DELETING FOOD ===");

        std::ofstream foodFile(m_foodConfigPath);
        if (!foodFile.is_open()) {
            Logger::Log("ERROR: Cannot open food config file for deletion: " + m_foodConfigPath);
            return;
        }

        int newId = 0;
        for (size_t i = 0; i < allFoods.size(); ++i) {
            if (i == static_cast<size_t>(m_selectedFoodIndex)) {
                continue;
            }

            const Food* food = allFoods[i];
            foodFile << newId << " "
                << food->GetName() << " "
                << food->GetSymbol() << " "
                << food->GetColor() << " "
                << food->GetHungerRestore() << " "
                << food->GetHpRestore() << " "
                << food->GetSpawnWeight() << " "
                << food->GetExperience() << "\n";
            newId++;
        }

        foodFile.close();

        if (m_foodManager->LoadFromFile(m_foodConfigPath)) {
            Logger::Log("Food deleted successfully");

            const auto& newFoods = m_foodManager->GetAllFood();
            if (m_selectedFoodIndex >= static_cast<int>(newFoods.size())) {
                m_selectedFoodIndex = newFoods.empty() ? -1 : newFoods.size() - 1;
            }
        }
        else {
            Logger::Log("ERROR: Failed to reload food after deletion");
        }

    }
    catch (const std::exception& e) {
        Logger::Log("ERROR: Failed to delete food: " + std::string(e.what()));
    }
}

void WorldEditor::SaveEditedFoodField() {
    if (m_tempTileStringInput.empty()) return;

    if (m_foodState == FoodState::EDITING_FOOD) {
        try {
            switch (m_editingTileFieldIndex) {
            case 0:
                m_editedFoodName = m_tempTileStringInput;
                break;
            case 1:
                if (!m_tempTileStringInput.empty()) {
                    m_editedFoodSymbol = m_tempTileStringInput[0];
                }
                break;
            case 2:
            {
                int newColor = std::stoi(m_tempTileStringInput);
                m_editedFoodColor = std::clamp(newColor, 0, 15);
            }
            break;
            case 3:
            {
                int value = std::stoi(m_tempTileStringInput);
                m_editedHungerRestore = value;
            }
            break;
            case 4:
            {
                int value = std::stoi(m_tempTileStringInput);
                m_editedHpRestore = value;
            }
            break;
            case 5:
            {
                int value = std::stoi(m_tempTileStringInput);
                m_editedSpawnWeight = std::clamp(value, 1, 100);
            }
            break;
            case 6:
            {
                int value = std::stoi(m_tempTileStringInput);
                m_editedExperience = value;
            }
            break;
            }
        }
        catch (const std::exception& e) {
            Logger::Log("ERROR: Invalid food input: " + m_tempTileStringInput);
        }
    }
}

void WorldEditor::ResetTemplateData() {
    Logger::Log("=== RESETTING TEMPLATE DATA ===");

    // Сброс конфигурации мира (основные настройки)
    m_config.SetWorldName("New_World");
    m_config.SetWidth(50);
    m_config.SetHeight(25);
    m_config.SetSeed(12345);
    m_config.SetRandomGeneration(true);
    m_config.SetNoiseFrequency(0.5f);
    m_config.SetNeighborRadius(1);

    // Сброс настроек игрока
    m_config.SetPlayerStartX(25);
    m_config.SetPlayerStartY(12);
    m_config.SetPlayerMaxHP(100);
    m_config.SetPlayerMaxHunger(100);
    m_config.SetEnableHP(true);
    m_config.SetEnableHunger(true);
    m_config.SetEnableEnemies(false);
    m_config.SetEnemySpawnRate(5);

    // Сброс данных тайлов
    m_availableTileIds.clear();
    m_selectedTileIndex = 0;
    m_tileActionIndex = 0;
    m_tilesState = TilesState::MAIN_LIST;

    // Сброс значений нового тайла
    m_newTileName = "new_tile";
    m_newTileSymbol = 'A';
    m_newTileColor = 7;
    m_newTileLowlandProb = 10;
    m_newTilePlainsProb = 10;
    m_newTileMountainProb = 10;

    // Сброс значений редактируемого тайла
    m_editedTileName = "";
    m_editedTileSymbol = ' ';
    m_editedTileColor = 7;
    m_editedTileLowlandProb = 0;
    m_editedTilePlainsProb = 0;
    m_editedTileMountainProb = 0;

    // Сброс данных еды
    m_selectedFoodIndex = 0;
    m_foodActionIndex = 0;
    m_foodState = FoodState::MAIN_LIST;

    // Сброс значений новой еды
    m_newFoodName = "new_food";
    m_newFoodSymbol = '*';
    m_newFoodColor = 10;
    m_newHungerRestore = 10;
    m_newHpRestore = 5;
    m_newSpawnWeight = 1;
    m_newExperience = 5;

    // Сброс значений редактируемой еды
    m_editedFoodName = "";
    m_editedFoodSymbol = ' ';
    m_editedFoodColor = 10;
    m_editedHungerRestore = 0;
    m_editedHpRestore = 0;
    m_editedSpawnWeight = 1;
    m_editedExperience = 0;

    // Сброс данных клеточного автомата
    m_survivalRules.clear();
    m_birthRules.clear();
    m_deathRules.clear();

    // Сброс временных строк
    m_tempStringInput = "";
    m_tempTileStringInput = "";
    m_tempRuleInput = "";

    // Сброс состояния редактирования
    m_isEditingText = false;
    m_editingField = -1;
    m_editingTileField = false;
    m_editingTileFieldIndex = 0;
    m_editingRule = false;

    // Сброс выбранных полей и вкладок
    m_selectedField = 0;
    m_selectedButton = 0;
    m_currentTab = EditorTab::WORLD;
    m_prevTab = EditorTab::WORLD;

    Logger::Log("Template data has been reset");
}

void WorldEditor::ClearTemplate() {
    Logger::Log("=== CLEARING TEMPLATE ===");

    ResetTemplateData();

    ClearTemplateFiles();

    //if (m_tileManager) {
    //    m_tileManager->GetAllTiles().clear();
    //    LoadAvailableTiles();
    //}

    //if (m_foodManager) {
    //    // Очищаем еду
    //    m_foodManager->ClearAll();
    //    LoadAvailableFood();
    //}
}

void WorldEditor::ClearTemplateFiles() {
    if (m_editorMode != EditorMode::CREATE_TEMPLATE) {
        return;
    }

    Logger::Log("=== CLEANING UP TEMPLATE FILES FOR SLOT " + std::to_string(m_slot) + " ===");

    std::string templateDir = "templates/template" + std::to_string(m_slot);

    try {
        if (!fs::exists(templateDir)) {
            Logger::Log("Template directory doesn't exist: " + templateDir);
            return;
        }

        for (const auto& entry : fs::directory_iterator(templateDir)) {
            try {
                if (fs::is_regular_file(entry.path())) {
                    if (fs::remove(entry.path())) {
                        Logger::Log("Deleted: " + entry.path().filename().string());
                    }
                    else {
                        Logger::Log("WARNING: Could not delete: " + entry.path().filename().string());
                    }
                }
            }
            catch (const std::exception& e) {
                Logger::Log("ERROR deleting " + entry.path().string() + ": " + e.what());
            }
        }

        try {
            for (const auto& entry : fs::directory_iterator(templateDir)) {
                if (fs::is_directory(entry.path())) {
                    fs::remove_all(entry.path());
                    Logger::Log("Removed subdirectory: " + entry.path().filename().string());
                }
            }

            if (fs::remove(templateDir)) {
                Logger::Log("Removed template directory: " + templateDir);
            }
            else {
                Logger::Log("WARNING: Could not remove template directory (might not be empty)");

                int remainingFiles = 0;
                for (const auto& entry : fs::directory_iterator(templateDir)) {
                    remainingFiles++;
                    Logger::Log("File still exists: " + entry.path().string());
                }
                Logger::Log("Remaining files in directory: " + std::to_string(remainingFiles));
            }
        }
        catch (const std::exception& e) {
            Logger::Log("ERROR removing template directory: " + std::string(e.what()));
        }

        Logger::Log("=== TEMPLATE FILES CLEANED UP ===");
    }
    catch (const std::exception& e) {
        Logger::Log("ERROR clearing template files: " + std::string(e.what()));
    }
}

bool WorldEditor::IsNewTemplate() const {
    if (m_editorMode != EditorMode::CREATE_TEMPLATE) {
        return false;
    }

    std::string templateDir;
    if (m_editorMode == EditorMode::CREATE_TEMPLATE) {
        templateDir = "templates/template" + std::to_string(m_slot);
    }

    std::string infoFile = templateDir + "/template_info.txt";

    return !fs::exists(infoFile);
}

void WorldEditor::LoadCellularAutomatonRules() {
    Logger::Log("Loading cellular automaton rules...");

    m_survivalRules.clear();
    m_birthRules.clear();
    m_deathRules.clear();

    std::string automatonPath;
    if (m_editorMode == EditorMode::CREATE_TEMPLATE) {
        automatonPath = "templates/template" + std::to_string(m_slot) + "/cellular_automaton.cfg";
    }
    else {
        automatonPath = "saves/slot" + std::to_string(m_slot) + "/cellular_automaton.cfg";
    }

    std::ifstream file(automatonPath);
    if (!file.is_open()) {
        Logger::Log("No cellular automaton config found, creating default");
        UpdateCellularRulesFromTiles();
        return;
    }

    std::string line;
    char currentTile = '\0';

    while (std::getline(file, line)) {
        size_t commentPos = line.find("//");
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }

        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);

        if (line.empty()) {
            continue;
        }

        if (line.length() == 1) {
            currentTile = line[0];
            Logger::Log("Loading rules for tile: '" + std::string(1, currentTile) + "'");
            continue;
        }

        size_t eqPos = line.find('=');
        if (eqPos != std::string::npos) {
            std::string key = line.substr(0, eqPos);
            std::string value = line.substr(eqPos + 1);

            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);

            if (currentTile == '\0') {
                Logger::Log("WARNING: Rule found without tile definition: " + line);
                continue;
            }

            if (key == "survival") {
                m_survivalRules[currentTile] = value;
                Logger::Log("  Survival rule: " + value);
            }
            else if (key == "birth") {
                m_birthRules[currentTile] = value;
                Logger::Log("  Birth rule: " + value);
            }
            else if (key == "death") {
                m_deathRules[currentTile] = value;
                Logger::Log("  Death rule: " + value);
            }
            else {
                Logger::Log("WARNING: Unknown rule type: " + key);
            }
        }
    }

    file.close();
    Logger::Log("Cellular automaton rules loaded: " +
        std::to_string(m_survivalRules.size()) + " survival rules, " +
        std::to_string(m_birthRules.size()) + " birth rules, " +
        std::to_string(m_deathRules.size()) + " death rules");
}

void WorldEditor::UpdateCellularRulesFromTiles() {
    Logger::Log("Updating cellular automaton rules from tiles...");

    m_survivalRules.clear();
    m_birthRules.clear();
    m_deathRules.clear();

    if (!m_tileManager) return;

    const auto& allTiles = m_tileManager->GetAllTiles();

    for (const auto& pair : allTiles) {
        const TileType& tile = pair.second;
        char character = tile.GetCharacter();

        if (character == ' ' || character == 0 ||
            tile.GetName() == "air" || tile.GetName() == "border") {
            continue;
        }

        m_survivalRules[character] = "";
        m_birthRules[character] = "";
        m_deathRules[character] = "";
    }

    Logger::Log("Cellular automaton rules updated for " +
        std::to_string(m_survivalRules.size()) + " tiles");
}

void WorldEditor::SaveCellularAutomatonRules() {
    std::string automatonPath;
    if (m_editorMode == EditorMode::CREATE_TEMPLATE) {
        automatonPath = "templates/template" + std::to_string(m_slot) + "/cellular_automaton.cfg";
    }
    else {
        automatonPath = "saves/slot" + std::to_string(m_slot) + "/cellular_automaton.cfg";
    }

    std::ofstream file(automatonPath);
    if (!file.is_open()) {
        Logger::Log("ERROR: Cannot open cellular automaton config for writing: " + automatonPath);
        return;
    }

    std::set<char> allTiles;
    for (const auto& pair : m_survivalRules) allTiles.insert(pair.first);
    for (const auto& pair : m_birthRules) allTiles.insert(pair.first);
    for (const auto& pair : m_deathRules) allTiles.insert(pair.first);

    for (char tileChar : allTiles) {
        file << tileChar << "\n";

        auto survivalIt = m_survivalRules.find(tileChar);
        auto birthIt = m_birthRules.find(tileChar);
        auto deathIt = m_deathRules.find(tileChar);

        bool hasAnyRule = false;

        if (survivalIt != m_survivalRules.end() && !survivalIt->second.empty()) {
            file << "survival=" << survivalIt->second << "\n";
            hasAnyRule = true;
        }

        if (birthIt != m_birthRules.end() && !birthIt->second.empty()) {
            file << "birth=" << birthIt->second << "\n";
            hasAnyRule = true;
        }

        if (deathIt != m_deathRules.end() && !deathIt->second.empty()) {
            file << "death=" << deathIt->second << "\n";
            hasAnyRule = true;
        }

        if (hasAnyRule) {
            file << "\n";
        }
    }

    file.close();
    Logger::Log("Saved cellular automaton rules to: " + automatonPath);

    std::ifstream checkFile(automatonPath);
    if (checkFile.is_open()) {
        std::string line;
        Logger::Log("=== Cellular Automaton Config Content ===");
        while (std::getline(checkFile, line)) {
            Logger::Log(line);
        }
        Logger::Log("=== End of Config ===");
        checkFile.close();
    }
}

void WorldEditor::HandleCellularInput() {
    if (!m_inputManager) return;

    m_prevRuleInput = m_tempRuleInput;
    m_prevCellularSelectedTileIndex = m_selectedField / 3;

    if (m_editingRule) {
        HandleRuleEditInput();
        return;
    }

    int tileCount = 0;
    for (int tileId : m_availableTileIds) {
        TileType* tile = m_tileManager->GetTileType(tileId);
        if (tile && tile->GetCharacter() != ' ') {
            tileCount++;
        }
    }

    if (tileCount == 0) {
        if (m_inputManager->IsMenuSelect()) {
            m_currentTab = EditorTab::TILES;
            m_needFullRedraw = true;
        }
        return;
    }

    int totalRules = tileCount * 3;

    int prevSelectedField = m_selectedField;
    int prevSelectedTileIndex = m_selectedField / 3;
    int prevRuleType = prevSelectedField % 3;

    if (m_inputManager->IsMenuUp() || m_inputManager->IsKeyPressed('W')) {
        if (m_selectedField > 0) {
            m_selectedField--;
            m_needFullRedraw = true;
        }
    }
    else if (m_inputManager->IsMenuDown() || m_inputManager->IsKeyPressed('S')) {
        if (m_selectedField < totalRules - 1) {
            m_selectedField++;
            m_needFullRedraw = true;
        }
    }
    else if (m_inputManager->IsMenuSelect()) {
        StartEditingSelectedRule();
        m_needFullRedraw = true;
    }
    else if (m_inputManager->IsMenuBack()) {
        m_shouldReturn = true;
    }

    int currentTileIndex = m_selectedField / 3;
    int currentRuleType = m_selectedField % 3;

    if ((m_inputManager->IsMenuDown() || m_inputManager->IsKeyPressed('S')) &&
        currentRuleType == 2 &&
        prevRuleType == 2 && 
        currentTileIndex < tileCount - 1) {
        m_selectedField = (currentTileIndex + 1) * 3;
        m_needFullRedraw = true;
    }
    else if ((m_inputManager->IsMenuUp() || m_inputManager->IsKeyPressed('W')) &&
        currentRuleType == 0 &&
        prevRuleType == 0 &&
        currentTileIndex > 0) {
        m_selectedField = (currentTileIndex - 1) * 3 + 2;
        m_needFullRedraw = true;
    }

    if (prevSelectedField != m_selectedField) {
        if ((m_selectedField / 3) != prevSelectedTileIndex) {
            m_needFullRedraw = true;
        }
    }
}

void WorldEditor::StartEditingSelectedRule() {
    std::vector<char> tileChars;
    for (int tileId : m_availableTileIds) {
        TileType* tile = m_tileManager->GetTileType(tileId);
        if (tile && tile->GetCharacter() != ' ') {
            tileChars.push_back(tile->GetCharacter());
        }
    }

    if (tileChars.empty()) return;

    int tileIndex = m_selectedField / 3;
    int ruleType = m_selectedField % 3;

    if (tileIndex >= static_cast<int>(tileChars.size())) {
        return;
    }

    m_selectedTileIndex = tileIndex;
    m_selectedTileForRules = tileChars[tileIndex];
    m_selectedRuleType = ruleType;

    std::string* currentRule = nullptr;
    switch (m_selectedRuleType) {
    case 0:
        currentRule = &m_survivalRules[m_selectedTileForRules];
        break;
    case 1:
        currentRule = &m_birthRules[m_selectedTileForRules];
        break;
    case 2:
        currentRule = &m_deathRules[m_selectedTileForRules];
        break;
    }

    m_tempRuleInput = currentRule ? *currentRule : "";
    m_cursorPos = m_tempRuleInput.length();
    m_editingRule = true;
    m_needFullRedraw = true;

    Logger::Log("Editing rule for tile '" + std::string(1, m_selectedTileForRules) +
        "', rule type: " + std::to_string(m_selectedRuleType) +
        ", current value: '" + m_tempRuleInput + "'" +
        ", cursor position: " + std::to_string(m_cursorPos));
}

void WorldEditor::StartEditingRuleForSelectedTile() {
    std::vector<char> tileChars;
    for (int tileId : m_availableTileIds) {
        TileType* tile = m_tileManager->GetTileType(tileId);
        if (tile && tile->GetCharacter() != ' ') {
            tileChars.push_back(tile->GetCharacter());
        }
    }

    if (m_selectedField < 0 || m_selectedField >= static_cast<int>(tileChars.size())) {
        return;
    }

    m_selectedTileForRules = tileChars[m_selectedField];
    m_selectedRuleType = 0;
    m_cellularState = CellularAutomatonState::EDITING_RULES;

    std::string* currentRule = nullptr;
    switch (m_selectedRuleType) {
    case 0: currentRule = &m_survivalRules[m_selectedTileForRules]; break;
    case 1: currentRule = &m_birthRules[m_selectedTileForRules]; break;
    case 2: currentRule = &m_deathRules[m_selectedTileForRules]; break;
    }

    m_tempRuleInput = currentRule ? *currentRule : "";
    m_editingRule = true;
    m_needFullRedraw = true;
}

void WorldEditor::HandleCellularMainInput() {
    if (m_inputManager->IsMenuUp()) {
        SelectPreviousOption();
    }
    else if (m_inputManager->IsMenuDown()) {
        SelectNextOption();
    }
    else if (m_inputManager->IsMenuSelect()) {
        if (m_selectedButton == 0) {
            std::vector<char> tileChars;
            for (int tileId : m_availableTileIds) {
                TileType* tile = m_tileManager->GetTileType(tileId);
                if (tile && tile->GetCharacter() != ' ') {
                    tileChars.push_back(tile->GetCharacter());
                }
            }

            if (m_selectedField < tileChars.size()) {
                m_selectedTileForRules = tileChars[m_selectedField];
                m_cellularState = CellularAutomatonState::TILE_SELECTION;
                m_selectedField = 0;
                m_needFullRedraw = true;
            }
        }
        else if (m_selectedButton == 1) {
            // Save/Apply
            SaveCellularAutomatonRules();
        }
        else if (m_selectedButton == 2) {
            m_shouldReturn = true;
        }
    }
    else if (m_inputManager->IsMenuBack()) {
        m_shouldReturn = true;
    }
}

void WorldEditor::HandleCellularTileSelectionInput() {
    if (m_inputManager->IsMenuUp()) {
        m_selectedField = (m_selectedField - 1 + 3) % 3;
        m_needFullRedraw = true;
    }
    else if (m_inputManager->IsMenuDown()) {
        m_selectedField = (m_selectedField + 1) % 3;
        m_needFullRedraw = true;
    }
    else if (m_inputManager->IsMenuSelect()) {
        m_selectedRuleType = m_selectedField;
        m_cellularState = CellularAutomatonState::EDITING_RULES;
        m_tempRuleInput = "";
        m_editingRule = false;
        m_needFullRedraw = true;
    }
    else if (m_inputManager->IsMenuBack()) {
        m_cellularState = CellularAutomatonState::MAIN_LIST;
        m_needFullRedraw = true;
    }
}

void WorldEditor::HandleCellularEditingInput() {
    if (m_inputManager->IsMenuSelect() || m_inputManager->IsKeyPressed(VK_SPACE)) {
        StartEditingRule();
    }
    else if (m_inputManager->IsMenuBack()) {
        m_cellularState = CellularAutomatonState::TILE_SELECTION;
        m_needFullRedraw = true;
    }
}
void WorldEditor::StartEditingRule() {
    m_editingRule = true;

    std::string* currentRule = nullptr;
    switch (m_selectedRuleType) {
    case 0: currentRule = &m_survivalRules[m_selectedTileForRules]; break;
    case 1: currentRule = &m_birthRules[m_selectedTileForRules]; break;
    case 2: currentRule = &m_deathRules[m_selectedTileForRules]; break;
    }

    if (currentRule) {
        m_tempRuleInput = *currentRule;
        m_cursorPos = m_tempRuleInput.length();
    }
    else {
        m_tempRuleInput = "";
        m_cursorPos = 0;
    }

    m_needFullRedraw = true;
}

void WorldEditor::HandleTextInputGeneral() {
    if (IsCtrlPressed() && m_inputManager->IsKeyPressed('C')) {
        if (!m_tempStringInput.empty()) {
            CopyToClipboard(m_tempStringInput);
            Logger::Log("Copied to clipboard: " + m_tempStringInput);
        }
        return;
    }

    if (IsCtrlPressed() && m_inputManager->IsKeyPressed('V')) {
        std::string clipboardText = PasteFromClipboard();
        if (!clipboardText.empty()) {
            m_tempStringInput += clipboardText;
            Logger::Log("Pasted from clipboard: " + clipboardText);
            m_needFullRedraw = true;
        }
        return;
    }

    if (IsCtrlPressed() && m_inputManager->IsKeyPressed('X')) {
        if (!m_tempStringInput.empty()) {
            CopyToClipboard(m_tempStringInput);
            m_tempStringInput.clear();
            Logger::Log("Cut to clipboard");
            m_needFullRedraw = true;
        }
        return;
    }

    for (char c = '0'; c <= '9'; c++) {
        if (m_inputManager->IsKeyPressed(c) && !IsCtrlPressed()) {
            m_tempStringInput += c;
            m_needFullRedraw = true;
            return;
        }
    }

    for (char c = 'A'; c <= 'Z'; c++) {
        if (m_inputManager->IsKeyPressed(c) && !IsCtrlPressed()) {
            m_tempStringInput += c;
            m_needFullRedraw = true;
            return;
        }
    }

    for (char c = 'a'; c <= 'z'; c++) {
        if (m_inputManager->IsKeyPressed(c) && !IsCtrlPressed()) {
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

    if (m_inputManager->IsKeyPressed(VK_DELETE)) {
        m_tempStringInput.clear();
        m_needFullRedraw = true;
        return;
    }
}

void WorldEditor::ApplyRuleEdit() {
    if (m_selectedTileForRules == '\0') return;

    switch (m_selectedRuleType) {
    case 0:
        if (!m_tempRuleInput.empty()) {
            m_survivalRules[m_selectedTileForRules] = m_tempRuleInput;
            Logger::Log("Saved survival rule for tile '" +
                std::string(1, m_selectedTileForRules) +
                "': " + m_tempRuleInput);
        }
        else {
            m_survivalRules.erase(m_selectedTileForRules);
            Logger::Log("Cleared survival rule for tile '" +
                std::string(1, m_selectedTileForRules) + "'");
        }
        break;
    case 1:
        if (!m_tempRuleInput.empty()) {
            m_birthRules[m_selectedTileForRules] = m_tempRuleInput;
            Logger::Log("Saved birth rule for tile '" +
                std::string(1, m_selectedTileForRules) +
                "': " + m_tempRuleInput);
        }
        else {
            m_birthRules.erase(m_selectedTileForRules);
            Logger::Log("Cleared birth rule for tile '" +
                std::string(1, m_selectedTileForRules) + "'");
        }
        break;
    case 2:
        if (!m_tempRuleInput.empty()) {
            m_deathRules[m_selectedTileForRules] = m_tempRuleInput;
            Logger::Log("Saved death rule for tile '" +
                std::string(1, m_selectedTileForRules) +
                "': " + m_tempRuleInput);
        }
        else {
            m_deathRules.erase(m_selectedTileForRules);
            Logger::Log("Cleared death rule for tile '" +
                std::string(1, m_selectedTileForRules) + "'");
        }
        break;
    }

    SaveCellularAutomatonRules();
}

void WorldEditor::SelectPreviousTab() {
    int current = static_cast<int>(m_currentTab);
    current = (current - 1 + 5) % 5;
    m_currentTab = static_cast<EditorTab>(current);

    if (m_currentTab == EditorTab::CELLULAR_AUTOMATON) {
        m_cellularState = CellularAutomatonState::MAIN_LIST;
        m_selectedRuleType = 0;
        m_editingRule = false;
        m_selectedTileForRules = '\0';
        m_tempRuleInput = "";
        m_selectedField = 0;
        m_cellularScrollOffset = 0;
    }

    if (m_currentTab == EditorTab::TILES) {
        m_tilesState = TilesState::MAIN_LIST;
        m_selectedTileIndex = 0;
        m_tileActionIndex = 0;
    }

    if (m_currentTab == EditorTab::FOOD) {
        m_foodState = FoodState::MAIN_LIST;
        m_selectedFoodIndex = 0;
        m_foodActionIndex = 0;
    }

    m_selectedField = 0;
    m_selectedButton = 0;
    m_isEditingText = false;
    m_editingField = -1;
    m_editingTileField = false;
    m_needFullRedraw = true;
}

bool WorldEditor::IsCtrlPressed() const {
    return (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
}

void WorldEditor::CopyToClipboard(const std::string& text) {
    if (text.empty()) return;

    if (OpenClipboard(nullptr)) {
        EmptyClipboard();

        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
        if (hMem) {
            char* pMem = static_cast<char*>(GlobalLock(hMem));
            if (pMem) {
                strcpy_s(pMem, text.size() + 1, text.c_str());
                GlobalUnlock(hMem);

                SetClipboardData(CF_TEXT, hMem);
            }
            GlobalFree(hMem);
        }
        CloseClipboard();
    }
}

std::string WorldEditor::PasteFromClipboard() {
    std::string result;

    if (OpenClipboard(nullptr)) {
        HANDLE hData = GetClipboardData(CF_TEXT);
        if (hData) {
            char* pszText = static_cast<char*>(GlobalLock(hData));
            if (pszText) {
                result = pszText;
                GlobalUnlock(hData);
            }
        }
        CloseClipboard();
    }

    return result;
}

void WorldEditor::HandleRuleEditInput() {
    if (m_inputManager->IsKeyPressed(VK_RETURN) || m_inputManager->IsKeyPressed(VK_SPACE)) {
        ApplyRuleEdit();
        m_editingRule = false;
        m_cellularState = CellularAutomatonState::MAIN_LIST;
        m_needFullRedraw = true;
        return;
    }

    if (m_inputManager->IsKeyPressed(VK_ESCAPE)) {
        m_editingRule = false;
        m_cellularState = CellularAutomatonState::MAIN_LIST;
        m_needFullRedraw = true;
        return;
    }

    // Стрелка влево - перемещение курсора влево
    if (m_inputManager->IsKeyPressed(VK_LEFT)) {
        if (m_cursorPos > 0) {
            m_cursorPos--;
            m_needFullRedraw = true;
        }
        return;
    }

    // Стрелка вправо - перемещение курсора вправо
    if (m_inputManager->IsKeyPressed(VK_RIGHT)) {
        if (m_cursorPos < m_tempRuleInput.length()) {
            m_cursorPos++;
            m_needFullRedraw = true;
        }
        return;
    }

    // Home - в начало строки
    if (m_inputManager->IsKeyPressed(VK_HOME)) {
        m_cursorPos = 0;
        m_needFullRedraw = true;
        return;
    }

    // End - в конец строки
    if (m_inputManager->IsKeyPressed(VK_END)) {
        m_cursorPos = m_tempRuleInput.length();
        m_needFullRedraw = true;
        return;
    }

    // Backspace - удаление символа слева от курсора
    if (m_inputManager->IsKeyPressed(VK_BACK)) {
        if (m_cursorPos > 0) {
            m_tempRuleInput.erase(m_cursorPos - 1, 1);
            m_cursorPos--;
            m_needFullRedraw = true;
        }
        return;
    }

    // Delete - удаление символа справа от курсора
    if (m_inputManager->IsKeyPressed(VK_DELETE)) {
        if (m_cursorPos < m_tempRuleInput.length()) {
            m_tempRuleInput.erase(m_cursorPos, 1);
            m_needFullRedraw = true;
        }
        return;
    }

    // Копирование и вставка
    if (IsCtrlPressed()) {
        if (m_inputManager->IsKeyPressed('C')) {
            if (!m_tempRuleInput.empty()) {
                CopyToClipboard(m_tempRuleInput);
                Logger::Log("Copied to clipboard: " + m_tempRuleInput.substr(0, 50) +
                    (m_tempRuleInput.length() > 50 ? "..." : ""));
            }
            return;
        }

        if (m_inputManager->IsKeyPressed('V')) {
            std::string clipboardText = PasteFromClipboard();
            if (!clipboardText.empty()) {
                // Вставляем текст в позиции курсора
                m_tempRuleInput.insert(m_cursorPos, clipboardText);
                m_cursorPos += clipboardText.length();
                Logger::Log("Pasted from clipboard at position " + std::to_string(m_cursorPos) +
                    ": " + clipboardText.substr(0, 50) +
                    (clipboardText.length() > 50 ? "..." : ""));
                m_needFullRedraw = true;
            }
            return;
        }

        if (m_inputManager->IsKeyPressed('X')) {
            if (!m_tempRuleInput.empty()) {
                CopyToClipboard(m_tempRuleInput);
                m_tempRuleInput.clear();
                m_cursorPos = 0;
                Logger::Log("Cut to clipboard");
                m_needFullRedraw = true;
            }
            return;
        }

        if (m_inputManager->IsKeyPressed('A')) {
            // Select all - устанавливаем курсор в конец
            m_cursorPos = m_tempRuleInput.length();
            m_needFullRedraw = true;
            return;
        }

        // Ctrl+Left/Right - перемещение по словам
        if (m_inputManager->IsKeyPressed(VK_LEFT)) {
            size_t newPos = m_cursorPos;
            while (newPos > 0 && isspace(m_tempRuleInput[newPos - 1])) {
                newPos--;
            }
            while (newPos > 0 && !isspace(m_tempRuleInput[newPos - 1])) {
                newPos--;
            }
            m_cursorPos = newPos;
            m_needFullRedraw = true;
            return;
        }

        if (m_inputManager->IsKeyPressed(VK_RIGHT)) {
            size_t newPos = m_cursorPos;
            size_t len = m_tempRuleInput.length();

            while (newPos < len && !isspace(m_tempRuleInput[newPos])) {
                newPos++;
            }
            while (newPos < len && isspace(m_tempRuleInput[newPos])) {
                newPos++;
            }
            m_cursorPos = newPos;
            m_needFullRedraw = true;
            return;
        }
    }

    bool shiftPressed = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    bool capsLockOn = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;

    for (char c = '0'; c <= '9'; c++) {
        if (m_inputManager->IsKeyPressed(c)) {
            char symbol = c;
            if (shiftPressed) {
                static const std::unordered_map<char, char> shiftDigits = {
                    {'0', ')'}, {'1', '!'}, {'2', '@'}, {'3', '#'}, {'4', '$'},
                    {'5', '%'}, {'6', '^'}, {'7', '&'}, {'8', '*'}, {'9', '('}
                };
                auto it = shiftDigits.find(c);
                if (it != shiftDigits.end()) {
                    symbol = it->second;
                }
            }
            m_tempRuleInput.insert(m_cursorPos, 1, symbol);
            m_cursorPos++;
            m_needFullRedraw = true;
            return;
        }
    }

    for (int vkCode = 'A'; vkCode <= 'Z'; vkCode++) {
        if (m_inputManager->IsKeyPressed(vkCode)) {
            char symbol = static_cast<char>(vkCode);
            if (!(shiftPressed ^ capsLockOn)) {
                symbol = symbol + 32;
            }
            m_tempRuleInput.insert(m_cursorPos, 1, symbol);
            m_cursorPos++;
            m_needFullRedraw = true;
            return;
        }
    }

    static const std::unordered_map<int, std::pair<char, char>> specialKeys = {
        {VK_OEM_MINUS, {'-', '_'}},
        {VK_OEM_PLUS, {'=', '+'}},
        {VK_OEM_1, {';', ':'}},
        {VK_OEM_2, {'/', '?'}},
        {VK_OEM_3, {'`', '~'}},
        {VK_OEM_4, {'[', '{'}},
        {VK_OEM_5, {'\\', '|'}},
        {VK_OEM_6, {']', '}'}},
        {VK_OEM_7, {'\'', '"'}},
        {VK_OEM_COMMA, {',', '<'}},
        {VK_OEM_PERIOD, {'.', '>'}},
        {VK_SPACE, {' ', ' '}},
    };

    for (const auto& [vkCode, chars] : specialKeys) {
        if (m_inputManager->IsKeyPressed(vkCode)) {
            char symbol = shiftPressed ? chars.second : chars.first;
            m_tempRuleInput.insert(m_cursorPos, 1, symbol);
            m_cursorPos++;
            m_needFullRedraw = true;
            return;
        }
    }

    for (int i = 32; i <= 255; i++) {
        if (m_inputManager->IsKeyPressed(i)) {
            if ((i >= '0' && i <= '9') || (i >= 'A' && i <= 'Z')) {
                continue;
            }

            bool isSpecialVK = false;
            for (const auto& [vkCode, _] : specialKeys) {
                if (vkCode == i) {
                    isSpecialVK = true;
                    break;
                }
            }
            if (isSpecialVK) {
                continue;
            }

            m_tempRuleInput.insert(m_cursorPos, 1, static_cast<char>(i));
            m_cursorPos++;
            m_needFullRedraw = true;
            return;
        }
    }
}

bool WorldEditor::SaveCellularAutomatonConfigPreserve(const std::string& directory) {
    std::string automatonConfigPath = directory + "/cellular_automaton.cfg";

    if (fs::exists(automatonConfigPath)) {
        Logger::Log("Cellular automaton config already exists at: " + automatonConfigPath);

        std::ifstream checkFile(automatonConfigPath);
        std::string line;
        bool hasContent = false;
        while (std::getline(checkFile, line)) {
            if (!line.empty() && line[0] != '#') {
                hasContent = true;
                break;
            }
        }
        checkFile.close();

        if (hasContent) {
            Logger::Log("Config has content, preserving it");
            return true;
        }
        else {
            Logger::Log("Config is empty, will create new");
        }
    }

    return SaveCellularAutomatonConfig(directory);
}

void WorldEditor::UpdateCellularRulesForTile(char oldSymbol, char newSymbol) {
    Logger::Log("Updating cellular automaton rules for symbol change: '" +
        std::string(1, oldSymbol) + "' -> '" + std::string(1, newSymbol) + "'");

    auto survivalIt = m_survivalRules.find(oldSymbol);
    auto birthIt = m_birthRules.find(oldSymbol);
    auto deathIt = m_deathRules.find(oldSymbol);

    bool hasRules = (survivalIt != m_survivalRules.end() && !survivalIt->second.empty()) ||
        (birthIt != m_birthRules.end() && !birthIt->second.empty()) ||
        (deathIt != m_deathRules.end() && !deathIt->second.empty());

    if (!hasRules) {
        Logger::Log("No rules found for old symbol '" + std::string(1, oldSymbol) + "', nothing to update");
        return;
    }

    if (survivalIt != m_survivalRules.end()) {
        m_survivalRules[newSymbol] = survivalIt->second;
        m_survivalRules.erase(oldSymbol);
        Logger::Log("Moved survival rule: '" + survivalIt->second + "'");
    }

    if (birthIt != m_birthRules.end()) {
        m_birthRules[newSymbol] = birthIt->second;
        m_birthRules.erase(oldSymbol);
        Logger::Log("Moved birth rule: '" + birthIt->second + "'");
    }

    if (deathIt != m_deathRules.end()) {
        m_deathRules[newSymbol] = deathIt->second;
        m_deathRules.erase(oldSymbol);
        Logger::Log("Moved death rule: '" + deathIt->second + "'");
    }

    UpdateSymbolInAllRules(oldSymbol, newSymbol);

    Logger::Log("Cellular automaton rules updated for symbol change");
}

void WorldEditor::UpdateSymbolInAllRules(char oldSymbol, char newSymbol) {
    UpdateSymbolInRuleSet(m_survivalRules, oldSymbol, newSymbol);
    UpdateSymbolInRuleSet(m_birthRules, oldSymbol, newSymbol);
    UpdateSymbolInRuleSet(m_deathRules, oldSymbol, newSymbol);
}

void WorldEditor::UpdateSymbolInRuleSet(std::unordered_map<char, std::string>& ruleSet, char oldSymbol, char newSymbol) {
    for (auto& pair : ruleSet) {
        std::string& rule = pair.second;
        size_t pos = 0;
        while ((pos = rule.find(oldSymbol, pos)) != std::string::npos) {
            rule.replace(pos, 1, 1, newSymbol);
            pos += 1;
        }
    }
}