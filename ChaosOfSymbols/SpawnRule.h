#pragma once
#include <unordered_set>

struct SpawnRule {
    int tileId;
    float probability;
    std::unordered_set<char> allowedBaseTiles;

    bool CanSpawnOn(char baseTile) const {
        return allowedBaseTiles.empty() ||
            allowedBaseTiles.find(baseTile) != allowedBaseTiles.end();
    }
};