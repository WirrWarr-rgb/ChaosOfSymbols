#include <fstream>
#include <sstream>
#include "FoodManager.h"
#include "Logger.h"

FoodManager::FoodManager() : m_totalSpawnWeight(0) {
}

FoodManager::~FoodManager() {
    for (auto food : m_foods) {
        delete food;
    }
    m_foods.clear();
    m_foodMap.clear();
}

/// <summary>
/// Загрузка конфигурации еды из файла и создание объектов Food
/// </summary>
/// <param name="filename"></param>
/// <returns></returns>
bool FoodManager::LoadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        Logger::Log("ERROR: Failed to open food config: " + filename);
        return false;
    }

    Logger::Log("=== LOADED FOOD SUMMARY ===\n");
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        int id, color, hunger, hp, weight, experience = 0;
        std::string name;
        char symbol;

        if (iss >> id >> name >> symbol >> color >> hunger >> hp >> weight >> experience) {
            Food* food = new Food(id, name, symbol, color, hunger, hp, weight, experience);
            m_foods.push_back(food);
            m_foodMap[id] = food;
            Logger::Log("Loaded food: " + name + " (ID: " + std::to_string(id) +
                ", XP: " + std::to_string(experience) + ")");
        }
        else {
            iss.clear();
            iss.str(line);
            if (iss >> id >> name >> symbol >> color >> hunger >> hp >> weight) {
                experience = hunger / 2 + hp * 2;
                Food* food = new Food(id, name, symbol, color, hunger, hp, weight, experience);
                m_foods.push_back(food);
                m_foodMap[id] = food;
                Logger::Log("Loaded food: " + name + " (ID: " + std::to_string(id) +
                    ", Auto-XP: " + std::to_string(experience) + ")");
            }
        }
    }

    CalculateSpawnWeights();
    Logger::Log("\nFood manager loaded " + std::to_string(m_foods.size()) + " food types");
    Logger::Log("\n=== END LOADED FOOD SUMMARY ===\n");
    return true;
}

/// <summary>
/// Получение объекта еды по его id
/// </summary>
const Food* FoodManager::GetFood(int id) const {
    auto it = m_foodMap.find(id);
    return (it != m_foodMap.end()) ? it->second : nullptr;
}

/// <summary>
/// Возвращает случайный объект еды с учетом весов спавна
/// </summary>
/// <returns></returns>
const Food* FoodManager::GetRandomFood() const {
    if (m_foods.empty() || m_totalSpawnWeight == 0)
        return nullptr;

    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, m_totalSpawnWeight);

    int randomWeight = dis(gen);
    int cumulativeWeight = 0;

    for (const auto& food : m_foods) {
        cumulativeWeight += food->GetSpawnWeight();
        if (randomWeight <= cumulativeWeight) {
            return food;
        }
    }

    return m_foods.back();
}

/// <summary>
/// Вычисление общего веса спавна для всех типов еды (для вероятностного выбора)
/// </summary>
void FoodManager::CalculateSpawnWeights() {
    m_totalSpawnWeight = 0;
    for (const auto& food : m_foods) {
        m_totalSpawnWeight += food->GetSpawnWeight();
    }
}