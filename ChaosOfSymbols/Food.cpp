#include "Food.h"

Food::Food()
    : m_id(0), m_name("Unknown"), m_symbol('?'), m_color(7),
    m_hungerRestore(0), m_hpRestore(0), m_spawnWeight(1) {
}

Food::Food(int id, const std::string& name, char symbol, int color,
    int hungerRestore, int hpRestore, int spawnWeight, int experience)
    : m_id(id), m_name(name), m_symbol(symbol), m_color(color),
    m_hungerRestore(hungerRestore), m_hpRestore(hpRestore),
    m_spawnWeight(spawnWeight), m_experience(experience) {
}