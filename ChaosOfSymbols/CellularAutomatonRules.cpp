#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include "CellularAutomatonRules.h"
#include "Logger.h"

/// <summary>
/// Выполняет оценку правила клеточного автомата на основе counts соседей
/// </summary>
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

/// <summary>
/// Парсит отдельное условие вида "count['#'] >= 2" и проверяет его выполнение
/// </summary>
bool RuleParser::parseCondition(const std::string& condition, const std::unordered_map<char, int>& counts) const {
    std::string trimmed = condition;
    trimmed.erase(0, trimmed.find_first_not_of(" \t"));
    trimmed.erase(trimmed.find_last_not_of(" \t") + 1);

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

/// <summary>
/// Загружает правила клеточного автомата из конфигурационного файла
/// </summary>
/// <param name="filename"></param>
/// <returns></returns>
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

    while (std::getline(file, line)) {
        lineNumber++;

        size_t commentPos = line.find("//");
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }

        line.erase(0, line.find_first_not_of(" \t"));
        if (line.empty()) {
            continue;
        }

        line.erase(line.find_last_not_of(" \t") + 1);

        // Если строка состоит из одного символа - это новый тайл
        if (line.length() == 1) {
            // Сохраняем предыдущее правило, если оно было
            if (currentTile != '\0' && hasTileDefinition) {
                m_rules[currentTile] = currentRule;
                Logger::Log("DEBUG: Saved rule for tile '" + std::string(1, currentTile) + "'");
            }

            currentTile = line[0];
            currentRule = CellRule();
            hasTileDefinition = true;
            Logger::Log("DEBUG: Started new tile '" + std::string(1, currentTile) + "'");
            continue;
        }

        size_t eqPos = line.find('=');
        if (eqPos == std::string::npos) {
            continue;
        }

        std::string key = line.substr(0, eqPos);
        std::string value = line.substr(eqPos + 1);

        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);

        if (!hasTileDefinition) {
            Logger::Log("ERROR: Rule without tile definition at line " + std::to_string(lineNumber));
            continue;
        }

        if (key == "survival") {
            currentRule.survivalRule = RuleParser::create(value);
        }
        else if (key == "birth") {
            currentRule.birthRule = RuleParser::create(value);
        }
        else if (key == "death") {
            currentRule.deathRule = RuleParser::create(value);
        }
        else {
            Logger::Log("WARNING: Unknown key: " + key);
        }
    }

    // Сохраняем последнее правило после окончания файла
    if (currentTile != '\0' && hasTileDefinition) {
        m_rules[currentTile] = currentRule;
        Logger::Log("DEBUG: Saved final rule for tile '" + std::string(1, currentTile) + "'");
    }

    file.close();
    LogRulesSummary();

    // Дополнительная отладочная информация
    Logger::Log("DEBUG: Total rules loaded: " + std::to_string(m_rules.size()));
    for (const auto& pair : m_rules) {
        Logger::Log("DEBUG: Rule for '" + std::string(1, pair.first) + "' is in map");
    }

    return !m_rules.empty();
}

/// <summary>
/// Получает правила для конкретного типа тайла
/// </summary>
const CellRule* CellularAutomatonConfig::GetRule(char tileChar) const {
    std::unordered_map<char, CellRule>::const_iterator it = m_rules.find(tileChar);
    if (it != m_rules.end()) {
        return &it->second;
    }
    return nullptr;
}

/// <summary>
/// Логирует сводку всех загруженных правил для отладки
/// </summary>
void CellularAutomatonConfig::LogRulesSummary() const {
    Logger::Log("\n=== LOADED CELLULAR AUTOMATON RULES SUMMARY ===\n");

    for (const auto& pair : m_rules) {
        char tileChar = pair.first;
        const CellRule& rule = pair.second;

        std::string ruleInfo = "Tile '" + std::string(1, tileChar) + "': ";
        bool hasRules = false;

        if (rule.survivalRule) {
            ruleInfo += "\nSurvival=" + rule.survivalRule->getRuleString();
            hasRules = true;
        }
        else {
            ruleInfo += "\nSurvival=ERROR";
        }
        if (rule.birthRule) {
            if (hasRules) ruleInfo += ", ";
            ruleInfo += "\nBirth=" + rule.birthRule->getRuleString();
            hasRules = true;
        }
        else {
            ruleInfo += "\nBirth=ERROR";
        }
        if (rule.deathRule) {
            if (hasRules) ruleInfo += ", ";
            ruleInfo += "\nDeath=" + rule.deathRule->getRuleString();
            hasRules = true;
        }
        else {
            ruleInfo += "\nDeath=ERROR";
        }

        Logger::Log(ruleInfo);
    }

    Logger::Log("\n=== END CELLULAR AUTOMATON RULES SUMMARY ===\n");
}