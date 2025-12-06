#include "TileType.h"

TileType::TileType()
    : m_id(0), m_name("unknown"), m_character('?'), m_color(7),
    m_isPassable(true), m_isDestructible(false), m_damage(0) {
    m_zoneProbabilities = { 0, 0, 0 };
}

TileType::TileType(int id, const std::string& name, char character, int color,
    bool passable, bool destructible, int damage,
    int lowlandProb, int plainsProb, int mountainProb)
    : m_id(id), m_name(name), m_character(character), m_color(color),
    m_isPassable(passable), m_isDestructible(destructible), m_damage(damage) {
    m_zoneProbabilities = { lowlandProb, plainsProb, mountainProb };
}

void TileType::SetZoneProbabilities(int lowland, int plains, int mountain) {
    m_zoneProbabilities = { lowland, plains, mountain };
}

const std::vector<int>& TileType::GetZoneProbabilities() const {
    return m_zoneProbabilities;
}

int TileType::GetLowlandProbability() const {
    return m_zoneProbabilities.size() > 0 ? m_zoneProbabilities[0] : 0;
}

int TileType::GetPlainsProbability() const {
    return m_zoneProbabilities.size() > 1 ? m_zoneProbabilities[1] : 0;
}

int TileType::GetMountainProbability() const {
    return m_zoneProbabilities.size() > 2 ? m_zoneProbabilities[2] : 0;
}