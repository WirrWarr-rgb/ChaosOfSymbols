#include "TileType.h"

TileType::TileType(int id, const std::string& name, char character, int color,
    bool passable, bool destructible, int damage)
    : m_id(id), m_name(name), m_character(character), m_color(color),
    m_isPassable(passable), m_isDestructible(destructible), m_damage(damage) {}

