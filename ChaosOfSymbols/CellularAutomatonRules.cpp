#include "CellularAutomatonRules.h"
#include "Logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

bool RuleParser::evaluate(const std::unordered_map<char, int>& neighborCounts) const {
    if (m_ruleString.empty() || m_ruleString == "true") return true;
    if (m_ruleString == "false") return false;

    // Простой парсер для условий типа "count['#'] >= 2 && count['~'] == 1"
    size_t andPos = m_ruleString.find("&&");
    size_t orPos = m_ruleString.find("||");

    if (andPos != std::string::npos) {
        std::string left = m_ruleString.substr(0, andPos);
        std::string right = m_ruleString.substr(andPos + 2);
        return parseCondition(left, neighborCounts) && parseCondition(right, neighborCounts);
    }
    else if (orPos != std::string::npos) {
        std::string left = m_ruleString.substr(0, orPos);
        std::string right = m_ruleString.substr(orPos + 2);
        return parseCondition(left, neighborCounts) || parseCondition(right, neighborCounts);
    }
    else {
        return parseCondition(m_ruleString, neighborCounts);
    }
}

bool RuleParser::parseCondition(const std::string& condition, const std::unordered_map<char, int>& counts) const {
    std::string trimmed = condition;
    trimmed.erase(0, trimmed.find_first_not_of(" \t"));
    trimmed.erase(trimmed.find_last_not_of(" \t") + 1);

    // Парсим условие типа "count['#'] >= 2"
    size_t bracketStart = trimmed.find("count['");
    if (bracketStart == std::string::npos) return false;

    size_t bracketEnd = trimmed.find("']", bracketStart);
    if (bracketEnd == std::string::npos) return false;

    char tileChar = trimmed[bracketStart + 7]; // после "count['"
    std::string rest = trimmed.substr(bracketEnd + 2);

    // Убираем пробелы
    rest.erase(std::remove(rest.begin(), rest.end(), ' '), rest.end());

    // Парсим оператор и значение
    std::string op;
    std::string valueStr;

    for (size_t i = 0; i < rest.size(); ++i) {
        if (std::isdigit(rest[i])) {
            op = rest.substr(0, i);
            valueStr = rest.substr(i);
            break;
        }
    }

    if (op.empty() || valueStr.empty()) return false;

    int value = std::stoi(valueStr);
    int actualCount = 0;

    std::unordered_map<char, int>::const_iterator it = counts.find(tileChar);
    if (it != counts.end()) {
        actualCount = it->second;
    }

    if (op == "==") return actualCount == value;
    if (op == ">=") return actualCount >= value;
    if (op == "<=") return actualCount <= value;
    if (op == ">") return actualCount > value;
    if (op == "<") return actualCount < value;
    if (op == "!=") return actualCount != value;

    return false;
}

bool CellularAutomatonConfig::LoadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        Logger::Log("ERROR: Cellular automaton config not found: " + filename);
        return false;
    }

    m_rules.clear();
    std::string line;
    char currentTile = '\0';
    CellRule currentRule;
    int lineNumber = 0;
    bool hasTileDefinition = false;

    Logger::Log("=== PARSING CELLULAR AUTOMATON CONFIG ===");

    while (std::getline(file, line)) {
        lineNumber++;

        // Пропускаем пустые строки и комментарии
        if (line.empty() || line[0] == '#') {
            Logger::Log("Line " + std::to_string(lineNumber) + ": skipped (comment/empty)");
            continue;
        }

        // Убираем пробелы в начале и конце строки
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);

        // Проверяем, является ли строка определением тайла (один символ в начале строки)
        if (line.length() == 1) {
            // Это определение нового тайла
            currentTile = line[0];
            hasTileDefinition = true;
            Logger::Log("Line " + std::to_string(lineNumber) + ": found tile definition: '" + std::string(1, currentTile) + "'");
            continue;
        }

        size_t eqPos = line.find('=');
        if (eqPos == std::string::npos) {
            Logger::Log("Line " + std::to_string(lineNumber) + ": no '=' found");
            continue;
        }

        std::string key = line.substr(0, eqPos);
        std::string value = line.substr(eqPos + 1);

        // Убираем пробелы
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);

        Logger::Log("Line " + std::to_string(lineNumber) + ": key='" + key + "', value='" + value + "'");

        if (!hasTileDefinition) {
            Logger::Log("ERROR: Rule without tile definition at line " + std::to_string(lineNumber));
            continue;
        }

        if (key == "survival") {
            currentRule.survivalRule = RuleParser::parse(value);
            Logger::Log("Set survival rule for tile '" + std::string(1, currentTile) + "': " + value);
        }
        else if (key == "birth") {
            currentRule.birthRule = RuleParser::parse(value);
            Logger::Log("Set birth rule for tile '" + std::string(1, currentTile) + "': " + value);

            // Сохраняем правило когда дошли до конца определения
            if (currentTile != '\0') {
                m_rules[currentTile] = currentRule;
                Logger::Log("SUCCESS: Registered rule for tile '" + std::string(1, currentTile) + "'");
                currentTile = '\0';
                currentRule = CellRule();
                hasTileDefinition = false;
            }
        }
        else if (key == "death") {
            currentRule.deathRule = RuleParser::parse(value);
            Logger::Log("Set death rule for tile '" + std::string(1, currentTile) + "': " + value);
        }
        else {
            Logger::Log("WARNING: Unknown key: " + key);
        }
    }

    file.close();
    Logger::Log("=== CELLULAR AUTOMATON CONFIG PARSING COMPLETE ===");
    Logger::Log("Total rules loaded: " + std::to_string(m_rules.size()));

    // Логируем все загруженные правила (без structured bindings)
    for (std::unordered_map<char, CellRule>::const_iterator it = m_rules.begin(); it != m_rules.end(); ++it) {
        char tileChar = it->first;
        Logger::Log("Rule for '" + std::string(1, tileChar) + "' is available");
    }

    return !m_rules.empty();
}

const CellRule* CellularAutomatonConfig::GetRule(char tileChar) const {
    std::unordered_map<char, CellRule>::const_iterator it = m_rules.find(tileChar);
    if (it != m_rules.end()) {
        return &it->second;
    }
    return nullptr;
}