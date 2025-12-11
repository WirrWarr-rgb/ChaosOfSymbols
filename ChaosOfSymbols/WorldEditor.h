#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "InputManager.h"
#include "SaveTypes.h"
#include "WorldConfig.h"
#include "SaveSystem.h"
#include "TileTypeManager.h"
#include "TileType.h"
#include "World.h"

enum class EditorTab {
    WORLD,
    PLAYER,
    TILES,
    CELLULAR_AUTOMATON,
    FOOD
};

enum class EditorMode {
    CREATE_WORLD,      // Создать мир и начать игру
    CREATE_TEMPLATE    // Создать только шаблон (для меню шаблонов)
};

enum class TilesState {
    MAIN_LIST,        // Основной список тайлов
    TILE_ACTIONS,     // Меню действий (Изменить/Удалить)
    EDITING_TILE,     // Редактирование тайла
    ADDING_TILE       // Добавление нового тайла
};

enum class CellularAutomatonState {
    MAIN_LIST,
    TILE_SELECTION,
    EDITING_RULES
};

enum class FoodState {
    MAIN_LIST,
    FOOD_ACTIONS,
    EDITING_FOOD,
    ADDING_FOOD
};

class WorldEditor {
public:
    WorldEditor(EditorMode mode, int slot, GameMode gameMode = GameMode::PROCEDURAL_GENERATION);
    void Initialize();
    void Update();
    void Render();
    void ProcessInput();

    bool ShouldReturnToSaves() const { return m_shouldReturn; }
    bool ShouldCreateWorld() const { return m_shouldCreate && m_editorMode == EditorMode::CREATE_WORLD; }
    bool ShouldCreateTemplate() const { return m_shouldCreate && m_editorMode == EditorMode::CREATE_TEMPLATE; }
    const WorldConfig& GetConfig() const { return m_config; }
    void ClearDefaultTiles();

    void UpdateHelpForCurrentSelection();
    void RegisterHelpSystemEntries();
    void RenderHelpPanel();
    bool LoadTemplateConfig(const WorldConfig& config);
    void SaveWorldConfiguration();
    bool SaveAllConfigurations(const std::string& directory);

    void LoadCellularAutomatonRules();
    void SaveCellularAutomatonRules();
    void UpdateCellularRulesFromTiles();
private:
    // Основные методы
    void CreateNewWorld();
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

    void RenderTileEditing(int startLine, bool isNewTile);
    void HandleTileEditInput();
    void SaveEditedTileField();
    void SaveNewTileField();
    void HandleProbabilityInput();

    int m_editedTileLowlandProb;
    int m_editedTilePlainsProb;
    int m_editedTileMountainProb;
    int m_newTileLowlandProb;
    int m_newTilePlainsProb;
    int m_newTileMountainProb;

    // Вкладка Tiles - полностью переработанная
    void HandleTileInput();
    void RenderTileList(int startLine);
    void RenderTileDetails();
    void StartEditingTile();
    void ApplyTileEdit();
    void AddNewTile();
    void DeleteSelectedTile();
    void ChangeTileColor(int delta);
    void LoadAvailableTiles();

    void HandleNeighborRadiusInput();

    void HandleTileActionsNavigation();
    void RenderTileActions(int startLine);

    void HandleSymbolInput();
    void HandleColorInput();
    void HandleTileNameInput();
    void CreateDefaultTiles();

    void CreateDirectoryForSlot(int slot);

    bool CreateTemplate(const std::string& templateName);
    bool LoadFromTemplate(int templateSlot);

    bool SaveWorldConfig(const std::string& directory);
    bool SavePlayerConfig(const std::string& directory);
    bool SaveTilesConfig(const std::string& directory);
    bool SaveCellularAutomatonConfig(const std::string& directory);
    bool SaveFoodConfig(const std::string& directory);
    bool SaveEnemiesConfig(const std::string& directory);

    void LoadAvailableFood();
    void RenderFoodList(int startLine);
    void RenderFoodActions(int startLine);
    void RenderFoodEditing(int startLine, bool isNewFood);
    void HandleFoodInput();
    void HandleFoodActionsNavigation();
    void StartEditingFood();
    void HandleFoodEditNavigation();
    void StartAddingFood();
    void HandleFoodEditInput();
    void SaveEditedFoodField();
    void SaveNewFoodField();
    void AddNewFood();
    void ApplyFoodEdit();
    void StartEditingFoodField();
    void DeleteSelectedFood();

    void HandleFoodColorInput();
    void HandleFoodNumericInput();
    void HandleFoodNameInput();
    void HandleFoodSymbolInput();
    void ResetTemplateData();
    void ClearTemplateFiles();
    void ClearTemplate();
    bool IsNewTemplate() const;
    void RenderCellularMainList(int startLine);
    void RenderCellularTileSelection(int startLine);
    void RenderCellularEditing(int startLine);
    void HandleCellularInput();
    void HandleCellularMainInput();
    void HandleCellularTileSelectionInput();
    void HandleCellularEditingInput();
    void HandleRuleEditInput();
    void ApplyRuleEdit();
    std::string GetCurrentFieldName() const;
    void StartEditingRule();
    void SelectPreviousTab();
    void StartEditingRuleForSelectedTile();
    void StartEditingSelectedRule();
    bool SaveCellularAutomatonConfigPreserve(const std::string& directory);
    std::string GetCurrentButtonName() const;
    std::string GetCurrentTabName() const;
    void UpdateCellularRulesForTile(char oldSymbol, char newSymbol);
    void UpdateSymbolInAllRules(char oldSymbol, char newSymbol);
    void UpdateSymbolInRuleSet(std::unordered_map<char, std::string>& ruleSet, char oldSymbol, char newSymbol);

    bool IsCtrlPressed() const;
    void CopyToClipboard(const std::string& text);
    std::string PasteFromClipboard();

    int m_cellularScrollOffset = 0;
    int m_visibleRulesCount = 7;

    int m_cursorPos;

    CellularAutomatonState m_cellularState;
    CellularAutomatonState m_prevCellularState;
    int m_selectedRuleType; // 0 - Survival, 1 - Birth, 2 - Death
    std::unordered_map<char, std::string> m_tileRules; // символ -> правила
    std::unordered_map<char, std::string> m_survivalRules;
    std::unordered_map<char, std::string> m_birthRules;
    std::unordered_map<char, std::string> m_deathRules;
    std::string m_tempRuleInput;
    int m_editingRuleIndex;
    bool m_editingRule;
    char m_selectedTileForRules;

    EditorMode m_editorMode;

    int m_tileActionIndex;  // Индекс выбранного действия (Edit/Delete/Back)
    std::string m_newTileName;  // Имя нового тайла
    char m_newTileSymbol;       // Символ нового тайла
    int m_newTileColor;         // Цвет нового тайла

    std::string m_editedTileName;
    char m_editedTileSymbol;
    int m_editedTileColor;

    std::string m_tilesConfigPath;

    // Навигация
    EditorTab m_currentTab;
    int m_selectedField;
    int m_selectedButton;

    // Состояние
    WorldConfig m_config;
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

    FoodState m_foodState;
    FoodState m_prevFoodState;

    // Индексы и выбор
    int m_selectedFoodIndex;
    int m_foodActionIndex;

    // Данные для редактирования еды
    std::string m_editedFoodName;
    char m_editedFoodSymbol;
    int m_editedFoodColor;
    int m_editedHungerRestore;
    int m_editedHpRestore;
    int m_editedSpawnWeight;
    int m_editedExperience;

    std::string m_prevRuleInput;
    int m_prevCellularSelectedTileIndex;
    char m_prevSelectedTileChar;

    // Данные для добавления новой еды
    std::string m_newFoodName;
    char m_newFoodSymbol;
    int m_newFoodColor;
    int m_newHungerRestore;
    int m_newHpRestore;
    int m_newSpawnWeight;
    int m_newExperience;

    // Менеджер еды
    std::unique_ptr<FoodManager> m_foodManager;
    std::vector<int> m_availableFoodIds;
    std::string m_foodConfigPath;
};