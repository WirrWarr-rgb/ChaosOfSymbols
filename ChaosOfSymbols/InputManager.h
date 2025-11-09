#pragma once
#include <windows.h>
#include <vector>
#include <chrono>
#include <unordered_map>

class InputManager {
public:
    InputManager();

    // Основные методы проверки ввода
    bool IsKeyPressed(int virtualKey);
    bool IsKeyHeld(int virtualKey);
    bool IsAnyKeyPressed(const std::vector<int>& virtualKeys);

    // Специальные методы для меню (с автоповтором)
    bool IsMenuUp();
    bool IsMenuDown();
    bool IsMenuSelect();
    bool IsMenuBack();

    void Update();

    void ClearState();
    void ClearSystemBuffer();

private:
    struct KeyState {
        bool currentState = false;
        bool previousState = false;
        std::chrono::steady_clock::time_point lastPressTime;
    };

    std::unordered_map<int, KeyState> m_keyStates;
    std::chrono::steady_clock::time_point m_lastUpdateTime;

    // Настройки
    static constexpr int MENU_REPEAT_DELAY_MS = 150;
    static constexpr int MENU_INITIAL_DELAY_MS = 300;
};