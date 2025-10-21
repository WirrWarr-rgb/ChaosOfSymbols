#pragma once
#include <string>

class Food {
public:
    Food();
    Food(int id, const std::string& name, char symbol, int color, int hungerRestore, int hpRestore, int spawnWeight, int experience = 0);

    int GetId() const { return m_id; }
    const std::string& GetName() const { return m_name; }
    char GetSymbol() const { return m_symbol; }
    int GetColor() const { return m_color; }
    int GetHungerRestore() const { return m_hungerRestore; }
    int GetHpRestore() const { return m_hpRestore; }
    int GetSpawnWeight() const { return m_spawnWeight; }
    int GetExperience() const { return m_experience; }

private:
    int m_id;
    std::string m_name;
    char m_symbol;
    int m_color;
    int m_hungerRestore;
    int m_hpRestore;
    int m_spawnWeight;
    int m_experience;
};