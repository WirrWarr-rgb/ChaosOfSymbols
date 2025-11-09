#pragma once
#include <string>

// Общие типы для системы сохранений
enum class GameMode {
    PROCEDURAL_GENERATION,
    PRELOADED_MAPS
};

struct SaveInfo {
    int slotNumber;
    std::string name;
    std::string creationDate;
    std::string lastPlayedDate;
    bool isEmpty;
    std::string savePath;
};