#pragma once
#include <string>

enum class GameMode {
    PROCEDURAL_GENERATION,
    PRELOADED_MAPS,
    FROM_TEMPLATE
};

struct SaveInfo {
    int slotNumber;
    std::string name;
    std::string creationDate;
    std::string lastPlayedDate;
    bool isEmpty;
    std::string savePath;
    GameMode gameMode;
    int templateId;

    SaveInfo() : slotNumber(0), isEmpty(true), gameMode(GameMode::PROCEDURAL_GENERATION), templateId(-1) {}

    std::string GetDisplayName() const {
        if (isEmpty) {
            return "Empty";
        }

        std::string display = name + ": ";

        if (!creationDate.empty()) {
            display += creationDate;
        }

        if (!lastPlayedDate.empty() && lastPlayedDate != creationDate) {
            display += " - " + lastPlayedDate;
        }

        return display;
    }
};