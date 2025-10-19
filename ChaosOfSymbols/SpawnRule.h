#pragma once
#include <unordered_set>
#include <vector>

struct SpawnRule {
    int tileId;
    std::vector<float> zoneProbabilities; // [������, �������, ����]
    char character;
};