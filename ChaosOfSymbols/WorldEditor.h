#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "InputManager.h"
#include "SaveTypes.h"
#include "WorldEditorConfig.h"
#include "SaveSystem.h"
#include "TileTypeManager.h"
#include "TileType.h"

enum class EditorTab {
    WORLD,
    PLAYER,
    TILES,
    CELLULAR_AUTOMATON,
    FOOD,
    ENEMIES,
    WIN,
    LOSE
};

// Состояния для вкладки Tiles
enum class TilesState {
    MAIN_LIST,
    EDITING_TILE,
    ADDING_TILE
};

class WorldEditor {
public:
    WorldEditor(GameMode mode, int slot);
    void Initialize();
    void Update();
    void Render();
    void ProcessInput();

    bool ShouldReturnToSaves() const { return m_shouldReturn; }
    bool ShouldCreateWorld() const { return m_shouldCreate; }
    const WorldEditorConfig& GetConfig() const { return m_config; }

private:
    // Основные методы
    void CreateNewWorld();
    void SaveWorldConfiguration();
    void RenderOnlyChanges();
    void RenderTabHeader();
    void RenderMenuItem(int line, const std::string& text, bool selected);
    void RenderEditField(int line, const std::string& label, const std::string& value, bool selected);
    void ClearLine(int line);
    bool NeedsRedraw() const;
    int GetMaxFields();

    // Вкладки
    void RenderWorldTab();
    void RenderPlayerTab();
    void RenderTilesTab();
    void RenderCellularAutomatonTab();
    void RenderFoodTab();
    void RenderEnemiesTab();
    void RenderWinTab();
    void RenderLoseTab();
    void RenderBottomButtons();

    // Обработка ввода
    void HandleStandardInput();
    void SelectNextTab();
    void SelectNextOption();
    void SelectPreviousOption();
    void StartEditing();
    void HandleTextInput();
    void ApplyEditedValue();
    void ConfirmSelection();
    void HandleTextInputGeneral();
    void HandleNumericInput();
    void HandleBooleanInput();
    void HandleFrequencyInput();
    void ChangeFieldValue(int delta);
    void HandleTileEditNavigation();
    void StartAddingTile();
    void StartEditingTileField();

    // Вспомогательные методы
    bool ShouldShowSeedField() const;
    int GetVisibleWorldFieldsCount() const;

    // Вкладка Tiles - полностью переработанная
    void HandleTileInput();
    void RenderTileList();
    void RenderTileDetails();
    void RenderTileEditing();
    void StartEditingTile();
    void HandleTileEditInput();
    void ApplyTileEdit();
    void AddNewTile();
    void DeleteSelectedTile();
    void ChangeTileColor(int delta);
    void LoadAvailableTiles();

    // Навигация
    EditorTab m_currentTab;
    int m_selectedField;
    int m_selectedButton;

    // Состояние
    WorldEditorConfig m_config;
    std::unique_ptr<SaveSystem> m_saveSystem;
    GameMode m_gameMode;
    int m_slot;
    bool m_shouldReturn;
    bool m_shouldCreate;

    // Ввод
    std::unique_ptr<InputManager> m_inputManager;

    // Текстовые поля для ввода
    std::string m_tempStringInput;
    bool m_isEditingText;
    int m_editingField;

    // Для частичной перерисовки
    EditorTab m_prevTab;
    int m_prevSelectedField;
    int m_prevSelectedButton;
    bool m_needFullRedraw;
    int m_prevFieldCount;

    // Константы
    static const int MAX_WORLD_WIDTH = 200;
    static const int MAX_WORLD_HEIGHT = 45;
    static const int MIN_WORLD_WIDTH = 1;
    static const int MIN_WORLD_HEIGHT = 1;

    // Для вкладки Tiles
    std::unique_ptr<TileTypeManager> m_tileManager;
    std::vector<int> m_availableTileIds;
    TilesState m_tilesState;
    TilesState m_prevTilesState;
    int m_selectedTileIndex;
    bool m_editingTileField;
    int m_editingTileFieldIndex;
    std::string m_tempTileStringInput;
};