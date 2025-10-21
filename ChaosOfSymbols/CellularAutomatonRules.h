#pragma once
#include <string>
#include <unordered_map>
#include <functional>
#include <vector>
#include <memory>

class RuleParser {
private:
    std::string m_ruleString;
    bool parseCondition(const std::string& condition, const std::unordered_map<char, int>& counts) const;

public:
    RuleParser() = default;
    RuleParser(const std::string& ruleStr) : m_ruleString(ruleStr) {}

    bool evaluate(const std::unordered_map<char, int>& neighborCounts) const;

    // ОСТАВЬ ТОЛЬКО ЭТОТ МЕТОД, удали другой parse
    static std::shared_ptr<RuleParser> create(const std::string& ruleStr) {
        auto parser = std::make_shared<RuleParser>();
        parser->m_ruleString = ruleStr;
        return parser;
    }

    const std::string& getRuleString() const {
        return m_ruleString;
    }
};

struct CellRule {
    std::shared_ptr<RuleParser> survivalRule;  // Исправь тип
    std::shared_ptr<RuleParser> birthRule;
    std::shared_ptr<RuleParser> deathRule;
};

class CellularAutomatonConfig {
private:
    std::unordered_map<char, CellRule> m_rules;  // Добавь m_rules

public:
    bool LoadFromFile(const std::string& filename);
    const CellRule* GetRule(char tileChar) const;
    bool HasRules() const { return !m_rules.empty(); }
    const std::unordered_map<char, CellRule>& GetAllRules() const { return m_rules; }
    void LogRulesSummary() const;
};