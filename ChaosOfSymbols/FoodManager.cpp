#include "FoodManager.h"
#include "Logger.h"
#include <fstream>
#include <sstream>

FoodManager::FoodManager() : m_totalSpawnWeight(0) {
}

FoodManager::~FoodManager() {
    for (auto food : m_foods) {
        delete food;
    }
    m_foods.clear();
    m_foodMap.clear();
}

bool FoodManager::LoadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        Logger::Log("ERROR: Failed to open food config: " + filename);
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        int id, color, hunger, hp, weight, experience = 0; // Добавьте experience
        std::string name;
        char symbol;

        // Читаем 7 значений вместо 6
        if (iss >> id >> name >> symbol >> color >> hunger >> hp >> weight >> experience) {
            Food* food = new Food(id, name, symbol, color, hunger, hp, weight, experience);
            m_foods.push_back(food);
            m_foodMap[id] = food;
            Logger::Log("Loaded food: " + name + " (ID: " + std::to_string(id) +
                ", XP: " + std::to_string(experience) + ")");
        }
        // Для обратной совместимости - если experience не указан
        else {
            // Сбрасываем поток и читаем без experience
            iss.clear();
            iss.str(line);
            if (iss >> id >> name >> symbol >> color >> hunger >> hp >> weight) {
                // Автоматически рассчитываем опыт на основе восстановления голода
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
    Logger::Log("Food manager loaded " + std::to_string(m_foods.size()) + " food types");
    return true;
}

const Food* FoodManager::GetFood(int id) const {
    auto it = m_foodMap.find(id);
    return (it != m_foodMap.end()) ? it->second : nullptr;
}

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

    return m_foods.back(); // fallback
}

void FoodManager::CalculateSpawnWeights() {
    m_totalSpawnWeight = 0;
    for (const auto& food : m_foods) {
        m_totalSpawnWeight += food->GetSpawnWeight();
    }
}