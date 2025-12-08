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
{
    HelpPanel::Initialize();
    RegisterHelpSystemEntries();

    // Определяем путь к конфигурации тайлов
    if (m_editorMode == EditorMode::CREATE_TEMPLATE) {
        m_tilesConfigPath = "templates/template" + std::to_string(slot) + "/tiles.json";

        // ЗАГРУЖАЕМ КОНФИГУРАЦИЮ ТЕМПЛАТА ПРИ РЕДАКТИРОВАНИИ
        TemplateSystem templateSystem;
        if (templateSystem.Initialize()) {
            if (templateSystem.LoadTemplate(slot, m_config)) {
                Logger::Log("Loaded existing template configuration for slot " + std::to_string(slot));

                // Также загружаем spawn rules из тайлов
                std::string templateDir = "templates/template" + std::to_string(slot);
                std::string spawnConfigPath = templateDir + "/world_spawn.cfg";

                // Загружаем spawn rules
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
                Logger::Log("No existing template found for slot " + std::to_string(slot) +
                    ", starting with default configuration");
            }
        }
    }
    else {
        m_tilesConfigPath = "saves/proceduralGeneration/slot" + std::to_string(slot) + "/tiles.json";

        // ЗАГРУЖАЕМ КОНФИГУРАЦИЮ СЕЙВА ПРИ РЕДАКТИРОВАНИИ МИРА
        std::string savePath;
        if (gameMode == GameMode::PROCEDURAL_GENERATION) {
            savePath = "saves/proceduralGeneration/slot" + std::to_string(slot);
        }
        else {
            savePath = "saves/preloadedMaps/slot" + std::to_string(slot);
        }

        if (fs::exists(savePath + "/world_gen.cfg")) {
            if (m_config.LoadFromDirectory(savePath)) {
                Logger::Log("Loaded existing world configuration for slot " + std::to_string(slot));
            }
        }
    }

    CreateDirectoryForSlot(slot);

    m_inputManager = std::make_unique<InputManager>();

    // Создаем менеджер тайлов
    Logger::Log("Creating TileTypeManager with path: " + m_tilesConfigPath);
    m_tileManager = std::make_unique<TileTypeManager>(m_tilesConfigPath);

    // Проверяем существование файла
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
        if (gameMode == GameMode::PROCEDURAL_GENERATION) {
            m_tilesConfigPath = "saves/proceduralGeneration/slot" + std::to_string(slot) + "/tiles.json";
            m_foodConfigPath = "saves/proceduralGeneration/slot" + std::to_string(slot) + "/food.cfg";
        }
        else {
            m_tilesConfigPath = "saves/preloadedMaps/slot" + std::to_string(slot) + "/tiles.json";
            m_foodConfigPath = "saves/preloadedMaps/slot" + std::to_string(slot) + "/food.cfg";
        }
    }

    // Создаем менеджер еды
    m_foodManager = std::make_unique<FoodManager>();

    // Загружаем существующую еду, если есть
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
    // Не показываем кнопки CREATE/BACK когда редактируем тайлы или еду
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
        Logger::Log("=== ADDING NEW TILE ===");
        Logger::Log("New tile ID: " + std::to_string(newId));
        Logger::Log("Name: " + m_newTileName);
        Logger::Log("Symbol: '" + std::string(1, m_newTileSymbol) + "'");
        Logger::Log("Color: " + std::to_string(m_newTileColor));
        Logger::Log("Probabilities: L=" + std::to_string(m_newTileLowlandProb) +
            " P=" + std::to_string(m_newTilePlainsProb) +
            " M=" + std::to_string(m_newTileMountainProb));

        TileType newTile(newId, m_newTileName, m_newTileSymbol, m_newTileColor,
            true, false, 0,
            m_newTileLowlandProb, m_newTilePlainsProb, m_newTileMountainProb);

        // Проверяем, есть ли уже такой символ
        bool symbolExists = false;
        const auto& allTiles = m_tileManager->GetAllTiles();
        for (const auto& pair : allTiles) {
            if (pair.second.GetCharacter() == m_newTileSymbol) {
                symbolExists = true;
                Logger::Log("WARNING: Symbol '" + std::string(1, m_newTileSymbol) +
                    "' already exists in tile: " + pair.second.GetName());
                break;
            }
        }

        m_tileManager->RegisterTileType(newTile);
        Logger::Log("Tile registered in manager");

        // Сохраняем файл
        if (m_tileManager->SaveToFile()) {
            Logger::Log("Tile saved to file successfully");
        }
        else {
            Logger::Log("ERROR: Failed to save tile to file");
        }

        // Загружаем заново
        Logger::Log("Reloading available tiles...");
        LoadAvailableTiles();

        Logger::Log("Added new tile: " + m_newTileName + " (ID: " + std::to_string(newId) +
            ") Symbol: '" + std::string(1, m_newTileSymbol) + "'" +
            " L:" + std::to_string(m_newTileLowlandProb) +
            " P:" + std::to_string(m_newTilePlainsProb) +
            " M:" + std::to_string(m_newTileMountainProb));

        // Сбрасываем значения для следующего тайла
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

    // Защищаем системные тайлы от удаления
    if (tileId == 0 || tileId == -1 || tileId == 2) {
        Logger::Log("Cannot delete system tiles (air/border)");
        return;
    }

    TileType* tile = m_tileManager->GetTileType(tileId);
    if (!tile) return;

    // Также защищаем по имени и символу
    if (tile->GetName() == "air" || tile->GetName() == "border" ||
        tile->GetName() == "stone_wall" || tile->GetCharacter() == ' ' ||
        tile->GetCharacter() == '#') {
        Logger::Log("Cannot delete system tiles (air/border)");
        return;
    }

    // Удаляем тайл из менеджера
    m_tileManager->RemoveTileType(tileId); // Нужно добавить этот метод в TileTypeManager

    m_tileManager->SaveToFile();

    LoadAvailableTiles();

    if (m_selectedTileIndex >= static_cast<int>(m_availableTileIds.size())) {
        m_selectedTileIndex = max(0, static_cast<int>(m_availableTileIds.size()) - 1);
    }

    Logger::Log("Deleted user tile with ID: " + std::to_string(tileId));
}

void WorldEditor::LoadAvailableTiles() {
    Logger::Log("=== LOADING AVAILABLE TILES ===");

    m_availableTileIds.clear();

    if (!m_tileManager) {
        Logger::Log("ERROR: No tile manager");
        return;
    }

    // Загружаем заново из файла
    if (!m_tileManager->LoadFromFile(m_tilesConfigPath)) {
        Logger::Log("ERROR: Failed to load tiles from: " + m_tilesConfigPath);
        return;
    }

    const auto& allTiles = m_tileManager->GetAllTiles();
    Logger::Log("Tile manager has " + std::to_string(allTiles.size()) + " tiles");

    // Выводим все тайлы для отладки
    for (const auto& pair : allTiles) {
        int tileId = pair.first;
        const TileType& tile = pair.second;

        Logger::Log("Found tile ID " + std::to_string(tileId) +
            ": '" + std::string(1, tile.GetCharacter()) + "' - " +
            tile.GetName() + " (color: " + std::to_string(tile.GetColor()) + ")");
    }

    // Фильтруем системные тайлы (воздух и границу)
    for (const auto& pair : allTiles) {
        int tileId = pair.first;
        const TileType& tile = pair.second;

        // Пропускаем только системные тайлы с отрицательными ID
        if (tileId < 0) {
            Logger::Log("Skipping system tile ID " + std::to_string(tileId) +
                ": '" + std::string(1, tile.GetCharacter()) + "' - " + tile.GetName());
            continue;
        }

        // Также пропускаем тайлы с пустым символом (пробел)
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

    // Выводим ID доступных тайлов
    std::string availableIds = "Available tile IDs: ";
    for (int id : m_availableTileIds) {
        availableIds += std::to_string(id) + " ";
    }
    Logger::Log(availableIds);

    Logger::Log("=== LOADING COMPLETE ===");
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

    const auto& allFoods = m_foodManager->GetAllFood(); // Получаем все еды

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
    RenderMenuItem(line, "- Edit", editSelected);
    line++;

    bool deleteSelected = (m_foodActionIndex == 1);
    RenderMenuItem(line, "- Delete", deleteSelected);
    line++;

    bool backSelected = (m_foodActionIndex == 2);
    RenderMenuItem(line, "- Back", backSelected);
}

void WorldEditor::RenderFoodEditing(int startLine, bool isNewFood) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    int line = startLine;

    // Заголовок
    rlutil::locate(4, line);
    SetConsoleTextAttribute(hConsole, 14);
    if (isNewFood) {
        std::cout << "Add New Food";
    }
    else {
        std::cout << "Edit Food";
    }
    SetConsoleTextAttribute(hConsole, 7);
    line += 2;

    // Очистка области
    for (int i = line; i < line + 7; ++i) {
        ClearLine(i);
    }

    // Рендерим левый столбец (поля 0-4)
    std::vector<std::string> leftFields = {
        "Name: ",
        "Symbol: ",
        "Color: ",
        "Hunger Restore: ",
        "HP Restore: "
    };

    // Рендерим правый столбец (поля 5-6)
    std::vector<std::string> rightFields = {
        "Spawn Weight: ",
        "Experience: "
    };

    // Рендерим левый столбец
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

    // Рендерим правый столбец (начинаем с 3 строки, чтобы выровнять с Hunger Restore и HP Restore)
    for (int i = 0; i < rightFields.size(); ++i) {
        int fieldIndex = i + 5; // поля 5 и 6
        bool isSelected = (fieldIndex == m_selectedField);
        bool isEditing = (m_isEditingText && m_editingTileFieldIndex == fieldIndex);

        rlutil::locate(40, line + 3 + i); // Начинаем с 3 строки

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

    // Кнопки Save и Cancel
    int buttonsStart = line + 6;
    ClearLine(buttonsStart);
    ClearLine(buttonsStart + 1);

    bool saveSelected = (m_selectedField == 7); // Поле 7 - Save
    RenderMenuItem(buttonsStart, "Save", saveSelected);

    bool cancelSelected = (m_selectedField == 8); // Поле 8 - Cancel
    RenderMenuItem(buttonsStart + 1, "Cancel", cancelSelected);

    SetConsoleTextAttribute(hConsole, 7);
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
            m_shouldReturn = true;
        }
        else if (m_currentTab == EditorTab::FOOD && m_foodState != FoodState::MAIN_LIST) {
            m_foodState = FoodState::MAIN_LIST;
            m_selectedField = 0;
            m_selectedButton = 0;
            m_needFullRedraw = true;
        }
        else if (m_currentTab == EditorTab::TILES && m_tilesState == TilesState::MAIN_LIST && m_selectedButton == 0) {
            m_shouldReturn = true;
        }
        else if (m_currentTab == EditorTab::TILES && m_tilesState != TilesState::MAIN_LIST) {
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
    case EditorTab::FOOD:
        HandleFoodInput();
        break;
    case EditorTab::WORLD:
    case EditorTab::PLAYER:
    case EditorTab::CELLULAR_AUTOMATON:
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
                // Сохраняем изменение
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
    case EditorTab::FOOD:
        if (m_foodState == FoodState::MAIN_LIST) {
            const auto& allFoods = m_foodManager->GetAllFood();
            if (allFoods.empty()) {
                return 1; // Только "Add New Food"
            }
            return static_cast<int>(allFoods.size()) + 1; // Все еды + "Add New Food"
        }
        else if (m_foodState == FoodState::FOOD_ACTIONS) {
            return 3;
        }
        else if (m_foodState == FoodState::EDITING_FOOD || m_foodState == FoodState::ADDING_FOOD) {
            return 7; // 7 полей + 2 кнопки
        }
        return 0;
    case EditorTab::CELLULAR_AUTOMATON:
        return 3;
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
            // Должно работать с тайлами, а не с едой!
            if (m_availableTileIds.empty()) {
                if (m_selectedField == 0) {
                    Logger::Log("Starting to add new tile...");
                    StartAddingTile(); // <-- Должно быть StartAddingTile()
                }
            }
            else {
                if (m_selectedField < static_cast<int>(m_availableTileIds.size())) {
                    m_selectedTileIndex = m_selectedField; // Выбор существующего тайла
                    m_tileActionIndex = 0;
                    m_tilesState = TilesState::TILE_ACTIONS;
                    m_needFullRedraw = true;
                }
                else if (m_selectedField == static_cast<int>(m_availableTileIds.size())) {
                    Logger::Log("Starting to add new tile...");
                    StartAddingTile(); // <-- Должно быть StartAddingTile()
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
            if (m_availableFoodIds.empty()) {
                if (m_selectedField == 0) {
                    Logger::Log("Starting to add new food...");
                    StartAddingFood();
                }
            }
            else {
                if (m_selectedField < static_cast<int>(m_availableFoodIds.size())) {
                    m_selectedFoodIndex = m_availableFoodIds[m_selectedField];
                    m_foodActionIndex = 0;
                    m_foodState = FoodState::FOOD_ACTIONS;
                    m_needFullRedraw = true;
                    Logger::Log("Selected food at index " + std::to_string(m_selectedField));
                }
                else if (m_selectedField == static_cast<int>(m_availableFoodIds.size())) {
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
        switch (m_foodActionIndex) {
        case 0: // Edit
            StartEditingFood();
            break;
        case 1: // Delete
            DeleteSelectedFood();
            m_foodState = FoodState::MAIN_LIST;
            m_selectedField = min(m_selectedField, static_cast<int>(m_availableFoodIds.size()) - 1);
            if (m_selectedField < 0) m_selectedField = 0;
            m_needFullRedraw = true;
            break;
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

    if (m_tileManager && m_tileManager->GetAllTiles().size() <= 2) { // только воздух и граница
        Logger::Log("WARNING: No user-defined tiles found!");
        Logger::Log("World will be created with only system tiles (air/border).");
        Logger::Log("Player can add tiles later in the Tiles tab.");
    }

    SaveWorldConfiguration();

    if (m_editorMode == EditorMode::CREATE_WORLD) {
        if (!m_saveSystem) {
            m_saveSystem = std::make_unique<SaveSystem>();
        }

        // Сохраняем все конфиги
        std::string savePath = m_saveSystem->GetSaveSlotPath(m_gameMode, m_slot);
        bool allSaved = SaveAllConfigurations(savePath);

        if (allSaved) {
            // Создаем инфо файл
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
        CreateTemplate(m_config.GetWorldName());
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

    // Проверяем наличие пользовательских тайлов
    int userTileCount = 0;
    if (m_tileManager) {
        const auto& allTiles = m_tileManager->GetAllTiles();
        for (const auto& pair : allTiles) {
            int tileId = pair.first;
            // Считаем только не-системные тайлы
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
    Logger::Log("Current food config path: " + m_foodConfigPath);

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
    std::ofstream destFile(foodConfigPath);

    if (!destFile.is_open()) {
        Logger::Log("ERROR: Cannot open food config file: " + foodConfigPath);
        return false;
    }

    // Получаем все текущие данные еды из менеджера
    const auto& allFoods = m_foodManager->GetAllFood();

    destFile << "# Food configuration\n";
    destFile << "# Format: ID Name Symbol Color HungerRestore HpRestore SpawnWeight Experience\n";

    if (allFoods.empty()) {
        // Если нет еды, создаем базовую конфигурацию
        Logger::Log("No food items found, creating default food config");
        destFile << "0 apple @ 12 20 10 5 10\n";
        destFile << "1 bread % 14 30 5 3 8\n";
        destFile << "2 meat & 4 50 30 2 20\n";
    }
    else {
        // Сохраняем все существующие элементы еды
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

    // Обновляем пути к конфигурации еды на основе текущего слота
    if (m_editorMode == EditorMode::CREATE_WORLD) {
        if (m_gameMode == GameMode::PROCEDURAL_GENERATION) {
            m_foodConfigPath = "saves/proceduralGeneration/slot" + std::to_string(m_slot) + "/food.cfg";
        }
        else {
            m_foodConfigPath = "saves/preloadedMaps/slot" + std::to_string(m_slot) + "/food.cfg";
        }
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

void WorldEditor::LoadAvailableFood() {
    Logger::Log("=== LOADING AVAILABLE FOOD ===");
    // Убираем m_availableFoodIds, так как будем использовать индексы напрямую
    // m_availableFoodIds.clear();

    if (!m_foodManager) {
        Logger::Log("ERROR: No food manager");
        return;
    }

    const auto& allFoods = m_foodManager->GetAllFood();
    Logger::Log("Food manager has " + std::to_string(allFoods.size()) + " food items");

    // Просто логируем загрузку, не сохраняем индексы
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
    }
    else if (m_inputManager->IsMenuDown()) {
        m_foodActionIndex = (m_foodActionIndex + 1) % 3;
    }
    else if (m_inputManager->IsMenuSelect()) {
        switch (m_foodActionIndex) {
        case 0: // Edit
            StartEditingFood();
            break;
        case 1: // Delete
            DeleteSelectedFood();
            m_foodState = FoodState::MAIN_LIST;
            m_selectedField = min(m_selectedField, static_cast<int>(m_availableFoodIds.size()) - 1);
            if (m_selectedField < 0) m_selectedField = 0;
            m_needFullRedraw = true;
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
    if (m_availableFoodIds.empty() || m_selectedFoodIndex >= static_cast<int>(allFoods.size())) return;

    const Food* food = allFoods[m_selectedFoodIndex];

    if (!food) return;

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
            if (m_selectedField == 4) m_selectedField = 4; // если был в левом столбце
            else m_selectedField = 5; // если был в правом
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
            m_foodState = FoodState::MAIN_LIST;
            m_selectedField = (m_foodState == FoodState::MAIN_LIST && m_availableFoodIds.empty()) ? 0 :
                (m_selectedFoodIndex < static_cast<int>(m_availableFoodIds.size()) ? m_selectedFoodIndex : 0);
            m_selectedButton = 0;
            m_needFullRedraw = true;
        }
    }
    else if (m_inputManager->IsMenuBack()) {
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

    if (m_inputManager->IsKeyPressed(VK_TAB)) {
        if (m_foodState == FoodState::ADDING_FOOD) {
            SaveNewFoodField();
        }
        else {
            SaveEditedFoodField();
        }

        m_editingTileFieldIndex = (m_editingTileFieldIndex + 1) % 7;
        m_tempTileStringInput = "";

        StartEditingFoodField();
        m_needFullRedraw = true;
        return;
    }

    if (m_editingTileFieldIndex >= 2 && m_editingTileFieldIndex <= 6) {
        HandleNumericInput();
    }
    else {
        switch (m_editingTileFieldIndex) {
        case 0:
            HandleTileNameInput();
            break;
        case 1:
            HandleSymbolInput();
            break;
        case 2:
            HandleColorInput();
            break;
        }
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
                m_editedHungerRestore = std::clamp(value, 0, 100);
            }
            break;
            case 4:
            {
                int value = std::stoi(m_tempTileStringInput);
                m_editedHpRestore = std::clamp(value, 0, 100);
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
                m_editedExperience = std::clamp(value, 0, 1000);
            }
            break;
            }
        }
        catch (const std::exception& e) {
            Logger::Log("ERROR: Invalid food input: " + m_tempTileStringInput);
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
            m_newHungerRestore = std::clamp(value, 0, 100);
        }
        break;
        case 4:
        {
            int value = std::stoi(m_tempTileStringInput);
            m_newHpRestore = std::clamp(value, 0, 100);
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
            m_newExperience = std::clamp(value, 0, 1000);
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

        // Сохраняем в файл
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

        // Перезагружаем менеджер еды
        m_foodManager->LoadFromFile(m_foodConfigPath);
        LoadAvailableFood();

        Logger::Log("Added new food: " + m_newFoodName +
            " (Hunger: " + std::to_string(m_newHungerRestore) +
            ", HP: " + std::to_string(m_newHpRestore) +
            ", XP: " + std::to_string(m_newExperience) + ")");

        // Сбрасываем значения для следующей еды
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
    // Для редактирования еды нужно перезаписать весь файл
    try {
        Logger::Log("=== EDITING FOOD ===");

        const auto& allFoods = m_foodManager->GetAllFood();
        if (m_selectedFoodIndex < 0 || m_selectedFoodIndex >= static_cast<int>(allFoods.size())) {
            Logger::Log("ERROR: Invalid food index");
            return;
        }

        // Открываем файл для полной перезаписи
        std::ofstream foodFile(m_foodConfigPath);
        if (!foodFile.is_open()) {
            Logger::Log("ERROR: Cannot open food config file for editing: " + m_foodConfigPath);
            return;
        }

        // Записываем все элементы еды, заменяя отредактированный
        for (size_t i = 0; i < allFoods.size(); ++i) {
            if (i == static_cast<size_t>(m_selectedFoodIndex)) {
                // Записываем отредактированный элемент
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
                // Записываем неизмененные элементы
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

        // Перезагружаем менеджер еды
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
    if (allFoods.empty() || m_selectedFoodIndex >= static_cast<int>(allFoods.size())) return;

    try {
        Logger::Log("=== DELETING FOOD ===");

        std::ofstream foodFile(m_foodConfigPath);
        if (!foodFile.is_open()) {
            Logger::Log("ERROR: Cannot open food config file for deletion: " + m_foodConfigPath);
            return;
        }

        // Записываем все элементы еды, кроме удаляемого
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

        // Перезагружаем менеджер еды
        m_foodManager->LoadFromFile(m_foodConfigPath);

        // Сбрасываем выбранный индекс
        if (m_selectedFoodIndex >= static_cast<int>(m_foodManager->GetAllFood().size())) {
            m_selectedFoodIndex = m_foodManager->GetAllFood().size() - 1;
        }

        Logger::Log("Food deleted successfully");

    }
    catch (const std::exception& e) {
        Logger::Log("ERROR: Failed to delete food: " + std::string(e.what()));
    }
}