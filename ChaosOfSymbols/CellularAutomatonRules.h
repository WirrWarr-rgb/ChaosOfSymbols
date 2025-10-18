#pragma once
#include <string>
#include <unordered_map>
#include <functional>
#include <vector>

class RuleParser {
private:
    std::string m_ruleString;

    bool parseCondition(const std::string& condition, const std::unordered_map<char, int>& counts) const;

public:
    RuleParser(const std::string& ruleStr) : m_ruleString(ruleStr) {}

    bool evaluate(const std::unordered_map<char, int>& neighborCounts) const;

    static std::function<bool(const std::unordered_map<char, int>&)> parse(const std::string& ruleStr) {
        return [parser = RuleParser(ruleStr)](const auto& counts) {
            return parser.evaluate(counts);
            };
    }
};

struct CellRule {
    std::function<bool(const std::unordered_map<char, int>&)> survivalRule;
    std::function<bool(const std::unordered_map<char, int>&)> birthRule;
    std::function<bool(const std::unordered_map<char, int>&)> deathRule;
};

class CellularAutomatonConfig {
private:
    std::unordered_map<char, CellRule> m_rules;

public:
    bool LoadFromFile(const std::string& filename);
    const CellRule* GetRule(char tileChar) const;
    bool HasRules() const { return !m_rules.empty(); }

    const std::unordered_map<char, CellRule>& GetAllRules() const { return m_rules; }
};