#include "InputManager.h"

InputManager::InputManager() {
    m_lastUpdateTime = std::chrono::steady_clock::now();

    // Инициализируем состояния всех клавиш, которые будем отслеживать
    std::vector<int> keysToTrack = {
        'W', 'S', 'A', 'D',
        VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT,
        VK_SPACE, VK_RETURN,
        'Q', VK_ESCAPE
    };

    for (int key : keysToTrack) {
        m_keyStates[key] = KeyState();
    }
}

void InputManager::Update() {
    auto currentTime = std::chrono::steady_clock::now();

    // Обновляем состояния всех отслеживаемых клавиш
    for (auto& pair : m_keyStates) {
        int key = pair.first;
        KeyState& state = pair.second;

        state.previousState = state.currentState;
        state.currentState = (GetAsyncKeyState(key) & 0x8000) != 0;

        // Если клавиша только что нажата, обновляем время нажатия
        if (state.currentState && !state.previousState) {
            state.lastPressTime = currentTime;
        }
    }

    m_lastUpdateTime = currentTime;
}

bool InputManager::IsKeyPressed(int virtualKey) {
    auto it = m_keyStates.find(virtualKey);
    if (it == m_keyStates.end()) {
        // Если клавиша не отслеживается, добавляем ее
        m_keyStates[virtualKey] = KeyState();
        return false;
    }

    KeyState& state = it->second;
    return state.currentState && !state.previousState;
}

bool InputManager::IsKeyHeld(int virtualKey) {
    auto it = m_keyStates.find(virtualKey);
    if (it == m_keyStates.end()) {
        m_keyStates[virtualKey] = KeyState();
        return false;
    }

    return it->second.currentState;
}

bool InputManager::IsAnyKeyPressed(const std::vector<int>& virtualKeys) {
    for (int key : virtualKeys) {
        if (IsKeyPressed(key)) {
            return true;
        }
    }
    return false;
}

bool InputManager::IsMenuUp() {
    std::vector<int> upKeys = { 'W', VK_UP };
    auto currentTime = std::chrono::steady_clock::now();

    for (int key : upKeys) {
        auto it = m_keyStates.find(key);
        if (it != m_keyStates.end()) {
            KeyState& state = it->second;

            if (state.currentState) {
                auto holdDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
                    currentTime - state.lastPressTime);

                // Первое нажатие
                if (!state.previousState) {
                    return true;
                }
                // Автоповтор после задержки
                else if (holdDuration.count() > MENU_INITIAL_DELAY_MS) {
                    auto sinceLastRepeat = std::chrono::duration_cast<std::chrono::milliseconds>(
                        currentTime - m_lastUpdateTime);
                    if (sinceLastRepeat.count() > MENU_REPEAT_DELAY_MS) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

bool InputManager::IsMenuDown() {
    std::vector<int> downKeys = { 'S', VK_DOWN };
    auto currentTime = std::chrono::steady_clock::now();

    for (int key : downKeys) {
        auto it = m_keyStates.find(key);
        if (it != m_keyStates.end()) {
            KeyState& state = it->second;

            if (state.currentState) {
                auto holdDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
                    currentTime - state.lastPressTime);

                // Первое нажатие
                if (!state.previousState) {
                    return true;
                }
                // Автоповтор после задержки
                else if (holdDuration.count() > MENU_INITIAL_DELAY_MS) {
                    auto sinceLastRepeat = std::chrono::duration_cast<std::chrono::milliseconds>(
                        currentTime - m_lastUpdateTime);
                    if (sinceLastRepeat.count() > MENU_REPEAT_DELAY_MS) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

bool InputManager::IsMenuSelect() {
    std::vector<int> selectKeys = { VK_SPACE, VK_RETURN };
    return IsAnyKeyPressed(selectKeys);
}

bool InputManager::IsMenuBack() {
    return IsKeyPressed('Q') || IsKeyPressed(VK_ESCAPE);
}

void InputManager::ClearState() {
    auto currentTime = std::chrono::steady_clock::now();

    // Сбрасываем все состояния клавиш
    for (auto& pair : m_keyStates) {
        KeyState& state = pair.second;
        state.currentState = false;
        state.previousState = false;
        state.lastPressTime = currentTime;
    }

    m_lastUpdateTime = currentTime;
}

void InputManager::ClearSystemBuffer() {
    // Очищаем системный буфер ввода Windows
    for (int i = 0; i < 256; i++) {
        // Двойной вызов чтобы сбросить состояние клавиш
        SHORT keyState = GetAsyncKeyState(i);
        keyState = GetAsyncKeyState(i); // Второй вызов для сброса
    }

    // Даем время системе обработать очистку
    Sleep(50);
}