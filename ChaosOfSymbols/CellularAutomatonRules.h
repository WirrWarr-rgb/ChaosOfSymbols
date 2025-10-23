#pragma once
#include <string>
#include <unordered_map>
#include <functional>
#include <vector>
#include <memory>

class RuleParser {
public:
    // Конструкторы
    RuleParser() = default;
    RuleParser(const std::string& ruleStr) : m_ruleString(ruleStr) {}

    // Публичные методы
    bool evaluate(const std::unordered_map<char, int>& neighborCounts) const;

    // Статические методы
    static std::shared_ptr<RuleParser> create(const std::string& ruleStr) {
        auto parser = std::make_shared<RuleParser>();
        parser->m_ruleString = ruleStr;
        return parser;
    }

    // Геттеры
    const std::string& getRuleString() const {
        return m_ruleString;
    }

private:
    // Приватные методы
    bool parseCondition(const std::string& condition, const std::unordered_map<char, int>& counts) const;

    // Приватные поля
    std::string m_ruleString;
};

struct CellRule {
    std::shared_ptr<RuleParser> survivalRule;
    std::shared_ptr<RuleParser> birthRule;
    std::shared_ptr<RuleParser> deathRule;
};

class CellularAutomatonConfig {
public:
    // Публичные методы
    bool LoadFromFile(const std::string& filename);
    void LogRulesSummary() const;

    // Геттеры
    const CellRule* GetRule(char tileChar) const;
    bool HasRules() const { return !m_rules.empty(); }
    const std::unordered_map<char, CellRule>& GetAllRules() const { return m_rules; }

private:
    // Приватные поля
    std::unordered_map<char, CellRule> m_rules;
};