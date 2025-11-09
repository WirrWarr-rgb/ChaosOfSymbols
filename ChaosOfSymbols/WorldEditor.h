#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "InputManager.h"
#include "SaveTypes.h"
#include "WorldEditorConfig.h"  // Используем WorldEditorConfig вместо WorldConfigData
#include "SaveSystem.h"

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
    void CreateNewWorld();  // ОДИН раз объявляем!
    void SaveWorldConfiguration();

    void RenderOnlyChanges();
    void RenderWorldTab();
    void RenderPlayerTab();
    void RenderTilesTab();
    void RenderCellularAutomatonTab();
    void RenderFoodTab();
    void RenderEnemiesTab();
    void RenderWinTab();
    void RenderLoseTab();
    void RenderBottomButtons();
    void RenderTabHeader();
    void RenderMenuItem(int line, const std::string& text, bool selected);
    void RenderEditField(int line, const std::string& label, const std::string& value, bool selected);
    void HandleNumericInput();  // Только числа 0-9
    void HandleBooleanInput();  // Переключение Yes/No
    void HandleFrequencyInput(); // Частота 0.0-1.0
    bool ShouldShowSeedField() const; // Показывать ли поле Seed
    int GetVisibleWorldFieldsCount() const; // Количество видимых полей

    void SelectNextTab();
    void SelectNextOption();
    void SelectPreviousOption();
    void StartEditing();
    void HandleTextInput();
    void ApplyEditedValue();
    void ChangeFieldValue(int delta);
    void ConfirmSelection();
    void HandleTextInputGeneral();

    void ClearLine(int line);

    int GetMaxFields();
    bool NeedsRedraw() const;

    // Навигация
    EditorTab m_currentTab;
    int m_selectedField;
    int m_selectedButton; // 0 = поля, 1 = Create, 2 = Back

    // Состояние - используем WorldEditorConfig вместо WorldConfigData
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

    int m_prevFieldCount = 0;
};