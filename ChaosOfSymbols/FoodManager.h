#pragma once
#include <vector>
#include <unordered_map>
#include <random>
#include "Food.h"

class FoodManager {
public:
    FoodManager();
    ~FoodManager();

    bool LoadFromFile(const std::string& filename = "config/food.cfg");
    const Food* GetFood(int id) const;
    const Food* GetRandomFood() const;

    int GetFoodCount() const { return m_foods.size(); }
    const std::vector<Food*>& GetAllFood() const { return m_foods; }

private:
    void CalculateSpawnWeights();

    std::vector<Food*> m_foods;
    std::unordered_map<int, Food*> m_foodMap;
    std::vector<int> m_spawnWeights;
    int m_totalSpawnWeight;
};