//#ifndef HELPSYSTEM_H
//#define HELPSYSTEM_H
//
//#include <string>
//#include <unordered_map>
//#include <vector>
//#include <memory>
//
//class HelpSystem {
//public:
//    static HelpSystem& GetInstance();
//
//    void AddHelpEntry(const std::string& itemId, const std::string& helpText);
//
//    std::string GetHelpText(const std::string& itemId) const;
//
//    bool HasHelpEntry(const std::string& itemId) const;
//
//    void ClearAllEntries();
//
//    void SetCurrentItem(const std::string& itemId);
//
//    std::string GetCurrentHelpText() const;
//
//    std::string GetCurrentItemId() const;
//
//    void ResetCurrentItem();
//
//private:
//    HelpSystem() = default;
//    ~HelpSystem() = default;
//
//    HelpSystem(const HelpSystem&) = delete;
//    HelpSystem& operator=(const HelpSystem&) = delete;
//
//    std::unordered_map<std::string, std::string> m_helpEntries;
//    std::string m_currentItemId;
//};
//
//#endif // HELPSYSTEM_H