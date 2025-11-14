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
    , m_tilesState(TilesState::MAIN_LIST)
    , m_prevTilesState(TilesState::MAIN_LIST)
    , m_selectedTileIndex(0)
    , m_editingTileField(false)
    , m_editingTileFieldIndex(0)
{
    m_inputManager = std::make_unique<InputManager>();

    m_tileManager = std::make_unique<TileTypeManager>();
    m_tileManager->LoadFromFile();
    LoadAvailableTiles();

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

    // Заголовок
    if (m_needFullRedraw) {
        rlutil::locate(2, 5);
        SetConsoleTextAttribute(hConsole, 14);
        std::cout << "Tile Configuration";
        SetConsoleTextAttribute(hConsole, 7);

        rlutil::locate(2, 6);
        std::cout << "------------------------------------------------------------";
    }

    // В зависимости от состояния показываем разные экраны
    switch (m_tilesState) {
    case TilesState::MAIN_LIST:
        RenderTileList();
        break;
    case TilesState::EDITING_TILE:
        RenderTileEditing();
        break;
    case TilesState::ADDING_TILE:
        RenderTileEditing(); // Используем тот же интерфейс для добавления
        break;
    }
}

void WorldEditor::RenderTileList() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    int line = 8;

    // Если нет тайлов, показываем сообщение
    if (m_availableTileIds.empty()) {
        rlutil::locate(4, line);
        std::cout << "No tiles available. Add your first tile to get started.";
        line += 2;
    }
    else {
        // Заголовок списка
        if (m_needFullRedraw) {
            rlutil::locate(4, line);
            std::cout << "Available Tiles:";
        }
        line++;

        // Список тайлов
        for (size_t i = 0; i < m_availableTileIds.size(); ++i) {
            int tileId = m_availableTileIds[i];
            TileType* tile = m_tileManager->GetTileType(tileId);

            if (!tile) continue;

            bool isSelected = (i == m_selectedField && m_selectedButton == 0);

            rlutil::locate(4, line + i);

            // Очищаем линию
            std::cout << "                                                                    ";
            rlutil::locate(4, line + i);

            if (isSelected) {
                SetConsoleTextAttribute(hConsole, 10);
                std::cout << "> ";
            }
            else {
                SetConsoleTextAttribute(hConsole, 7);
                std::cout << "  ";
            }

            // Символ с цветом
            SetConsoleTextAttribute(hConsole, tile->GetColor());
            std::cout << tile->GetCharacter();

            SetConsoleTextAttribute(hConsole, isSelected ? 10 : 7);
            std::cout << " - " << tile->GetName() << " (ID: " << tile->GetId() << ")";

            SetConsoleTextAttribute(hConsole, 7);
        }

        line += m_availableTileIds.size() + 1;
    }

    // Кнопки действий
    if (!m_availableTileIds.empty() && m_selectedField < static_cast<int>(m_availableTileIds.size())) {
        // Кнопка редактирования выбранного тайла
        bool editSelected = (m_selectedField == static_cast<int>(m_availableTileIds.size()) && m_selectedButton == 0);
        RenderMenuItem(line, "Edit Selected Tile", editSelected);
        line++;

        // Кнопка удаления выбранного тайла
        bool deleteSelected = (m_selectedField == static_cast<int>(m_availableTileIds.size()) + 1 && m_selectedButton == 0);
        RenderMenuItem(line, "Delete Selected Tile", deleteSelected);
        line++;
    }

    // Кнопка добавления нового тайла
    int addIndex = m_availableTileIds.empty() ? 0 :
        (m_availableTileIds.empty() ? 0 : static_cast<int>(m_availableTileIds.size()) +
            ((!m_availableTileIds.empty() && m_selectedField < static_cast<int>(m_availableTileIds.size())) ? 2 : 0));

    bool addSelected = (m_selectedField == addIndex && m_selectedButton == 0);
    RenderMenuItem(line, "+ Add New Tile", addSelected);
    line++;

    // Кнопка назад
    bool backSelected = (m_selectedField == addIndex + 1 && m_selectedButton == 0);
    RenderMenuItem(line, "Back", backSelected);

    // Детали выбранного тайла (если есть тайлы и выбран один)
    if (!m_availableTileIds.empty() && m_selectedField < static_cast<int>(m_availableTileIds.size())) {
        RenderTileDetails();
    }
}

void WorldEditor::RenderTileDetails() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    int detailLine = 8;
    int detailColumn = 40;

    int tileId = m_availableTileIds[m_selectedField];
    TileType* tile = m_tileManager->GetTileType(tileId);

    if (!tile) return;

    // Заголовок деталей
    rlutil::locate(detailColumn, detailLine);
    SetConsoleTextAttribute(hConsole, 14);
    std::cout << "Tile Properties:";

    rlutil::locate(detailColumn, detailLine + 1);
    SetConsoleTextAttribute(hConsole, 7);
    std::cout << "-------------------------";

    // Свойства тайла
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

    // Подсказки
    rlutil::locate(detailColumn, detailLine + 11);
    SetConsoleTextAttribute(hConsole, 8);
    std::cout << "A/D - Change color";

    rlutil::locate(detailColumn, detailLine + 12);
    std::cout << "ENTER - Edit properties";

    SetConsoleTextAttribute(hConsole, 7);
}

void WorldEditor::RenderTileEditing() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    int line = 8;

    // Заголовок редактирования
    rlutil::locate(4, line);
    SetConsoleTextAttribute(hConsole, 14);
    if (m_tilesState == TilesState::ADDING_TILE) {
        std::cout << "Add New Tile";
    }
    else {
        std::cout << "Edit Tile";
    }
    SetConsoleTextAttribute(hConsole, 7);

    line += 2;

    // Поля для редактирования
    std::vector<std::string> fields = {
        "Name: ",
        "Symbol: ",
        "Color: ",
        "Passable: ",
        "Destructible: ",
        "Damage: "
    };

    // Получаем текущий тайл или создаем временный для нового
    TileType* currentTile = nullptr;
    TileType tempTile(0, "", ' ', 7, true, false, 0);

    if (m_tilesState == TilesState::EDITING_TILE && m_selectedTileIndex < static_cast<int>(m_availableTileIds.size())) {
        int tileId = m_availableTileIds[m_selectedTileIndex];
        currentTile = m_tileManager->GetTileType(tileId);
    }

    for (int i = 0; i < fields.size(); ++i) {
        bool isSelected = (i == m_selectedField && m_selectedButton == 0);
        bool isEditing = (m_isEditingText && m_editingTileField && m_editingTileFieldIndex == i);

        rlutil::locate(6, line + i);

        // Очищаем линию
        std::cout << "                                                                    ";
        rlutil::locate(6, line + i);

        if (isSelected) {
            SetConsoleTextAttribute(hConsole, 10);
            std::cout << "> ";
        }
        else {
            SetConsoleTextAttribute(hConsole, 7);
            std::cout << "  ";
        }

        std::cout << fields[i];

        if (isEditing) {
            // Режим редактирования
            SetConsoleTextAttribute(hConsole, 11);
            std::cout << m_tempTileStringInput << "_";
        }
        else {
            // Режим отображения
            SetConsoleTextAttribute(hConsole, 7);

            // Отображаем текущее значение
            if (currentTile) {
                switch (i) {
                case 0: std::cout << currentTile->GetName(); break;
                case 1:
                    std::cout << "'";
                    SetConsoleTextAttribute(hConsole, currentTile->GetColor());
                    std::cout << currentTile->GetCharacter();
                    SetConsoleTextAttribute(hConsole, 7);
                    std::cout << "'";
                    break;
                case 2: std::cout << currentTile->GetColor(); break;
                case 3: std::cout << (currentTile->IsPassable() ? "Yes" : "No"); break;
                case 4: std::cout << (currentTile->IsDestructible() ? "Yes" : "No"); break;
                case 5: std::cout << currentTile->GetDamage(); break;
                }
            }
            else if (m_tilesState == TilesState::ADDING_TILE) {
                // Значения по умолчанию для нового тайла
                switch (i) {
                case 0: std::cout << "new_tile"; break;
                case 1: std::cout << "'A'"; break;
                case 2: std::cout << "7"; break;
                case 3: std::cout << "Yes"; break;
                case 4: std::cout << "No"; break;
                case 5: std::cout << "0"; break;
                }
            }
        }

        SetConsoleTextAttribute(hConsole, 7);
    }

    // Кнопки действий
    int buttonsStart = line + fields.size() + 1;

    bool saveSelected = (m_selectedField == static_cast<int>(fields.size()) && m_selectedButton == 0);
    RenderMenuItem(buttonsStart, "Save", saveSelected);

    bool cancelSelected = (m_selectedField == static_cast<int>(fields.size()) + 1 && m_selectedButton == 0);
    RenderMenuItem(buttonsStart + 1, "Cancel", cancelSelected);
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

    // Навигация по основному списку
    if (m_inputManager->IsMenuUp()) {
        if (m_selectedField > 0) {
            m_selectedField--;
        }
        else {
            m_selectedField = GetMaxFields() - 1;
        }
    }
    else if (m_inputManager->IsMenuDown()) {
        if (m_selectedField < GetMaxFields() - 1) {
            m_selectedField++;
        }
        else {
            m_selectedField = 0;
        }
    }
    else if (m_inputManager->IsMenuSelect()) {
        if (m_availableTileIds.empty()) {
            // Единственная кнопка - добавить
            if (m_selectedField == 0) {
                StartAddingTile();
            }
        }
        else {
            if (m_selectedField < static_cast<int>(m_availableTileIds.size())) {
                // Выбран существующий тайл - можно менять цвет
                ChangeTileColor(1); // Просто для примера - можно убрать
            }
            else if (m_selectedField == static_cast<int>(m_availableTileIds.size())) {
                // Редактировать выбранный тайл
                StartEditingTile();
            }
            else if (m_selectedField == static_cast<int>(m_availableTileIds.size()) + 1) {
                // Удалить выбранный тайл
                DeleteSelectedTile();
            }
            else if (m_selectedField == static_cast<int>(m_availableTileIds.size()) + 2) {
                // Добавить новый тайл
                StartAddingTile();
            }
            else if (m_selectedField == static_cast<int>(m_availableTileIds.size()) + 3) {
                // Назад
                m_shouldReturn = true;
            }
        }
    }
    else if (m_inputManager->IsKeyPressed('A') || m_inputManager->IsKeyPressed(VK_LEFT)) {
        // Изменение цвета выбранного тайла
        if (!m_availableTileIds.empty() && m_selectedField < static_cast<int>(m_availableTileIds.size())) {
            ChangeTileColor(-1);
        }
    }
    else if (m_inputManager->IsKeyPressed('D') || m_inputManager->IsKeyPressed(VK_RIGHT)) {
        // Изменение цвета выбранного тайла
        if (!m_availableTileIds.empty() && m_selectedField < static_cast<int>(m_availableTileIds.size())) {
            ChangeTileColor(1);
        }
    }
}

void WorldEditor::HandleTileEditNavigation() {
    if (m_inputManager->IsMenuUp()) {
        if (m_selectedField > 0) {
            m_selectedField--;
        }
    }
    else if (m_inputManager->IsMenuDown()) {
        int maxFields = 6 + 2; // 6 полей + 2 кнопки
        if (m_selectedField < maxFields - 1) {
            m_selectedField++;
        }
    }
    else if (m_inputManager->IsMenuSelect()) {
        if (m_selectedField < 6) {
            // Начало редактирования поля
            StartEditingTileField();
        }
        else if (m_selectedField == 6) {
            // Сохранить
            ApplyTileEdit();
            m_tilesState = TilesState::MAIN_LIST;
            m_selectedField = m_selectedTileIndex;
            m_needFullRedraw = true;
        }
        else if (m_selectedField == 7) {
            // Отмена
            m_tilesState = TilesState::MAIN_LIST;
            m_selectedField = m_selectedTileIndex;
            m_needFullRedraw = true;
        }
    }
    else if (m_inputManager->IsMenuBack()) {
        // Отмена по ESC
        m_tilesState = TilesState::MAIN_LIST;
        m_selectedField = m_selectedTileIndex;
        m_needFullRedraw = true;
    }
}

void WorldEditor::StartEditingTile() {
    if (m_availableTileIds.empty() || m_selectedField >= static_cast<int>(m_availableTileIds.size())) return;

    m_tilesState = TilesState::EDITING_TILE;
    m_selectedTileIndex = m_selectedField;
    m_selectedField = 0;
    m_editingTileField = false;
    m_needFullRedraw = true;
}

void WorldEditor::StartAddingTile() {
    m_tilesState = TilesState::ADDING_TILE;
    m_selectedTileIndex = -1;
    m_selectedField = 0;
    m_editingTileField = false;
    m_needFullRedraw = true;
}

void WorldEditor::StartEditingTileField() {
    m_isEditingText = true;
    m_editingTileField = true;
    m_editingTileFieldIndex = m_selectedField;
    m_tempTileStringInput = "";

    // Заполняем начальное значение
    if (m_tilesState == TilesState::EDITING_TILE && m_selectedTileIndex < static_cast<int>(m_availableTileIds.size())) {
        int tileId = m_availableTileIds[m_selectedTileIndex];
        TileType* tile = m_tileManager->GetTileType(tileId);

        if (tile) {
            switch (m_editingTileFieldIndex) {
            case 0: m_tempTileStringInput = tile->GetName(); break;
            case 1: m_tempTileStringInput = std::string(1, tile->GetCharacter()); break;
            case 2: m_tempTileStringInput = std::to_string(tile->GetColor()); break;
            case 3: m_tempTileStringInput = tile->IsPassable() ? "true" : "false"; break;
            case 4: m_tempTileStringInput = tile->IsDestructible() ? "true" : "false"; break;
            case 5: m_tempTileStringInput = std::to_string(tile->GetDamage()); break;
            }
        }
    }
    else if (m_tilesState == TilesState::ADDING_TILE) {
        // Значения по умолчанию для нового тайла
        switch (m_editingTileFieldIndex) {
        case 0: m_tempTileStringInput = "new_tile"; break;
        case 1: m_tempTileStringInput = "A"; break;
        case 2: m_tempTileStringInput = "7"; break;
        case 3: m_tempTileStringInput = "true"; break;
        case 4: m_tempTileStringInput = "false"; break;
        case 5: m_tempTileStringInput = "0"; break;
        }
    }
}

void WorldEditor::HandleTileEditInput() {
    if (m_inputManager->IsKeyPressed(VK_RETURN)) {
        ApplyTileEdit();
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
        // Переход к следующему полю
        m_editingTileFieldIndex = (m_editingTileFieldIndex + 1) % 6;
        m_tempTileStringInput = "";
        m_needFullRedraw = true;
        return;
    }

    // Обработка ввода в зависимости от типа поля
    switch (m_editingTileFieldIndex) {
    case 0: // Имя - текст
    case 1: // Символ - один символ
        HandleTextInputGeneral();
        break;
    case 2: // Цвет - числа
        HandleNumericInput();
        break;
    case 3: // Проходимость - true/false
    case 4: // Разрушаемость - true/false
        HandleBooleanInput();
        break;
    case 5: // Урон - числа (могут быть отрицательные)
        HandleNumericInput();
        break;
    }
}

void WorldEditor::ApplyTileEdit() {
    if (m_tempTileStringInput.empty()) return;

    int tileId = m_availableTileIds[m_selectedField];
    TileType* tile = m_tileManager->GetTileType(tileId);

    if (!tile) return;

    try {
        switch (m_editingTileFieldIndex) {
        case 0: // Имя
            // Для изменения имени нужно создать новый тайл
        {
            TileType newTile = *tile;
            newTile.SetName(m_tempTileStringInput);
            m_tileManager->RegisterTileType(newTile);
            // Обновляем список
            LoadAvailableTiles();
        }
        break;
        case 1: // Символ
            if (!m_tempTileStringInput.empty()) {
                TileType newTile = *tile;
                newTile.SetCharacter(m_tempTileStringInput[0]);
                m_tileManager->RegisterTileType(newTile);
                LoadAvailableTiles();
            }
            break;
        case 2: // Цвет
        {
            int newColor = std::stoi(m_tempTileStringInput);
            TileType newTile = *tile;
            newTile.SetColor(std::clamp(newColor, 0, 15));
            m_tileManager->RegisterTileType(newTile);
            LoadAvailableTiles();
        }
        break;
        case 3: // Проходимость
        {
            bool passable = (m_tempTileStringInput == "true");
            TileType newTile = *tile;
            newTile.SetPassable(passable);
            m_tileManager->RegisterTileType(newTile);
            LoadAvailableTiles();
        }
        break;
        case 4: // Разрушаемость
        {
            bool destructible = (m_tempTileStringInput == "true");
            TileType newTile = *tile;
            newTile.SetDestructible(destructible);
            m_tileManager->RegisterTileType(newTile);
            LoadAvailableTiles();
        }
        break;
        case 5: // Урон
        {
            int damage = std::stoi(m_tempTileStringInput);
            TileType newTile = *tile;
            newTile.SetDamage(damage);
            m_tileManager->RegisterTileType(newTile);
            LoadAvailableTiles();
        }
        break;
        }

        // Сохраняем изменения в файл
        m_tileManager->SaveToFile();

    }
    catch (const std::exception& e) {
        Logger::Log("ERROR: Invalid tile input: " + m_tempTileStringInput);
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
    // Находим максимальный ID и создаем новый тайл
    int newId = 0;
    for (int id : m_availableTileIds) {
        if (id >= newId) newId = id + 1;
    }

    // Создаем тайл с разумными значениями по умолчанию
    TileType newTile(newId, "new_tile_" + std::to_string(newId),
        static_cast<char>('A' + (newId % 26)), // A, B, C, ...
        7, // Белый цвет по умолчанию
        true, // Проходимый по умолчанию
        false, // Неразрушаемый по умолчанию
        0); // Без урона по умолчанию

    m_tileManager->RegisterTileType(newTile);
    m_tileManager->SaveToFile();

    LoadAvailableTiles();

    // Автоматически выбираем новый тайл для редактирования
    m_selectedField = static_cast<int>(m_availableTileIds.size()) - 1;
    m_needFullRedraw = true;

    Logger::Log("Added new tile with ID: " + std::to_string(newId));

    // Автоматически начинаем редактирование имени нового тайла
    StartEditingTileField();
}

void WorldEditor::DeleteSelectedTile() {
    if (m_availableTileIds.empty() || m_selectedField >= static_cast<int>(m_availableTileIds.size())) return;

    int tileId = m_availableTileIds[m_selectedField];

    // Не позволяем удалять базовые тайлы (ID 0, 1, 2)
    if (tileId <= 2) {
        Logger::Log("Cannot delete basic tiles (ID 0-2)");
        return;
    }

    // Удаляем тайл из менеджера
    // Note: Вам нужно добавить метод RemoveTile в TileTypeManager
    // m_tileManager->RemoveTile(tileId);

    // Временно: создаем новый список без удаленного тайла
    std::vector<int> newTileIds;
    for (int id : m_availableTileIds) {
        if (id != tileId) {
            newTileIds.push_back(id);
        }
    }
    m_availableTileIds = newTileIds;

    m_tileManager->SaveToFile();

    if (m_selectedField >= static_cast<int>(m_availableTileIds.size())) {
        m_selectedField = max(0, static_cast<int>(m_availableTileIds.size()) - 1);
    }

    m_needFullRedraw = true;
    Logger::Log("Deleted tile with ID: " + std::to_string(tileId));
}

void WorldEditor::LoadAvailableTiles() {
    m_availableTileIds.clear();

    if (!m_tileManager) return;

    const auto& allTiles = m_tileManager->GetAllTiles();
    for (const auto& pair : allTiles) {
        m_availableTileIds.push_back(pair.first);
    }

    // Сортируем по ID для удобства
    std::sort(m_availableTileIds.begin(), m_availableTileIds.end());
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
        if (m_editingTileField) {
            HandleTileInput();
        }
        else {
            HandleTextInput();
        }
        return;
    }

    // Обработка в зависимости от текущей вкладки
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
    case EditorTab::PLAYER:
        return 6;
    case EditorTab::TILES:
        if (m_tilesState == TilesState::MAIN_LIST) {
            if (m_availableTileIds.empty()) {
                return 2; // Сообщение + Add New Tile + Back
            }
            else {
                return static_cast<int>(m_availableTileIds.size()) + 4; // Тайлы + Edit + Delete + Add New + Back
            }
        }
        else {
            return 8; // 6 полей + Save + Cancel
        }
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