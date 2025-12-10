#include "InputManager.h"

InputManager::InputManager() {
    m_lastUpdateTime = std::chrono::steady_clock::now();

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

    for (auto& pair : m_keyStates) {
        int key = pair.first;
        KeyState& state = pair.second;

        state.previousState = state.currentState;
        state.currentState = (GetAsyncKeyState(key) & 0x8000) != 0;

        if (state.currentState && !state.previousState) {
            state.lastPressTime = currentTime;
        }
    }

    m_lastUpdateTime = currentTime;
}

bool InputManager::IsKeyPressed(int virtualKey) {
    // ¬сегда провер€ем текущее состо€ние клавиши
    bool currentState = (GetAsyncKeyState(virtualKey) & 0x8000) != 0;

    auto it = m_keyStates.find(virtualKey);
    if (it == m_keyStates.end()) {
        // ≈сли клавиша еще не отслеживалась, добавл€ем ее
        KeyState newState;
        newState.currentState = currentState;
        newState.previousState = false;
        newState.lastPressTime = m_lastUpdateTime;
        m_keyStates[virtualKey] = newState;
        return false;
    }

    KeyState& state = it->second;
    state.previousState = state.currentState;
    state.currentState = currentState;

    // ќбновл€ем врем€ нажати€
    if (state.currentState && !state.previousState) {
        state.lastPressTime = m_lastUpdateTime;
    }

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

                if (!state.previousState) {
                    return true;
                }
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

                if (!state.previousState) {
                    return true;
                }
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

    for (auto& pair : m_keyStates) {
        KeyState& state = pair.second;
        state.currentState = false;
        state.previousState = false;
        state.lastPressTime = currentTime;
    }

    m_lastUpdateTime = currentTime;
}

void InputManager::ClearSystemBuffer() {
    for (int i = 0; i < 256; i++) {
        SHORT keyState = GetAsyncKeyState(i);
        keyState = GetAsyncKeyState(i);
    }
    
    Sleep(50);
}