#pragma once
#include <unordered_set>
#include <vector>

struct SpawnRule {
    int tileId;
    char character;
    std::vector<float> zoneProbabilities; // [низины, равнины, горы]

    SpawnRule() : tileId(-1), character('?') {}
};