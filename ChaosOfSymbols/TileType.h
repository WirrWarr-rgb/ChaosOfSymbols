#pragma once
#include <string>
#include <vector>

class TileType {
public:
    // Конструкторы
    TileType();
    TileType(int id, const std::string& name, char character, int color,
        bool passable, bool destructible, int damage,
        int lowlandProb = 0, int plainsProb = 0, int mountainProb = 0);

    // Геттеры
    int GetId() const { return m_id; }
    std::string GetName() const { return m_name; }
    char GetCharacter() const { return m_character; }
    int GetColor() const { return m_color; }
    bool IsPassable() const { return m_isPassable; }
    bool IsDestructible() const { return m_isDestructible; }
    int GetDamage() const { return m_damage; }

    // Сеттеры
    void SetName(const std::string& name) { m_name = name; }
    void SetCharacter(char ch) { m_character = ch; }
    void SetColor(int color) { m_color = color; }
    void SetPassable(bool passable) { m_isPassable = passable; }
    void SetDestructible(bool destructible) { m_isDestructible = destructible; }
    void SetDamage(int damage) { m_damage = damage; }

    void SetZoneProbabilities(int lowland, int plains, int mountain);
    const std::vector<int>& GetZoneProbabilities() const;
    int GetLowlandProbability() const;
    int GetPlainsProbability() const;
    int GetMountainProbability() const;

private:
    int m_id;
    std::string m_name;
    char m_character;
    int m_color;
    bool m_isPassable;
    bool m_isDestructible;
    int m_damage;

    std::vector<int> m_zoneProbabilities; // [низины, равнины, горы]
};