#pragma once
#include <string>

class TileType {
private:
    int m_id;
    std::string m_name;
    char m_character;
    int m_color;
    bool m_isPassable;
    bool m_isDestructible;
    int m_damage;

public:
    TileType(int id = 0, const std::string& name = "unknown",
        char character = '?', int color = 15,
        bool passable = true, bool destructible = false, int damage = 0);

    int GetId() const { return m_id; }
    std::string GetName() const { return m_name; }
    char GetCharacter() const { return m_character; }
    int GetColor() const { return m_color; }
    bool IsPassable() const { return m_isPassable; }
    bool IsDestructible() const { return m_isDestructible; }
    int GetDamage() const { return m_damage; }

    void SetName(const std::string& name) { m_name = name; }
    void SetCharacter(char ch) { m_character = ch; }
    void SetColor(int color) { m_color = color; }
    void SetPassable(bool passable) { m_isPassable = passable; }
    void SetDestructible(bool destructible) { m_isDestructible = destructible; }
    void SetDamage(int damage) { m_damage = damage; }
};