#pragma once
#include <vector>
#include <unordered_map>
#include <random>
#include "Food.h"

class FoodManager {
public:
    FoodManager(const std::string& configPath = "");
    ~FoodManager();

    const Food* GetFood(int id) const;
    const Food* GetRandomFood() const;
    bool LoadFromFile(const std::string& filename);
    bool LoadFromFile();

    int GetFoodCount() const { return m_foods.size(); }
    const std::vector<Food*>& GetAllFood() const { return m_foods; }

private:
    void CalculateSpawnWeights();

    std::string m_configPath;

    std::vector<Food*> m_foods;
    std::unordered_map<int, Food*> m_foodMap;
    std::vector<int> m_spawnWeights;
    int m_totalSpawnWeight;
};