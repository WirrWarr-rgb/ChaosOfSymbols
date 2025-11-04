#pragma once
#include <string>
#include <unordered_map>

class PlayerConfig {
public:
    PlayerConfig();
    PlayerConfig(const std::string& configPath);

    bool LoadConfig();
    void SetDefaults();

    // Getters
    int GetDefaultPlayerX() const { return m_defaultPlayerX; }
    int GetDefaultPlayerY() const { return m_defaultPlayerY; }
    int GetMaxHP() const { return m_maxHP; }
    int GetMaxHunger() const { return m_maxHunger; }
    bool IsHPEnabled() const { return m_enableHP; }
    bool IsHungerEnabled() const { return m_enableHunger; }
    int GetBaseXP() const { return m_baseXP; }
    float GetXPMultiplier() const { return m_xpMultiplier; }
    int GetMoveCooldownMs() const { return m_moveCooldownMs; }

    // Setters
    void SetDefaultPlayerX(int x) { m_defaultPlayerX = x; }
    void SetDefaultPlayerY(int y) { m_defaultPlayerY = y; }
    void SetMaxHP(int hp) { m_maxHP = hp; }
    void SetMaxHunger(int hunger) { m_maxHunger = hunger; }
    void SetHPEnabled(bool enabled) { m_enableHP = enabled; }
    void SetHungerEnabled(bool enabled) { m_enableHunger = enabled; }

private:
    bool ParseKeyValue(const std::string& key, const std::string& value);

private:
    std::string m_configPath;

    int m_defaultPlayerX;
    int m_defaultPlayerY;
    int m_maxHP;
    int m_maxHunger;
    bool m_enableHP;
    bool m_enableHunger;
    int m_baseXP;
    float m_xpMultiplier;
    int m_moveCooldownMs;
};