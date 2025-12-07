#pragma once
#include <string>
#include <unordered_map>
#include <vector>

class HelpSystem {
public:
    static HelpSystem& GetInstance();

    HelpSystem(const HelpSystem&) = delete;
    HelpSystem& operator=(const HelpSystem&) = delete;

    void AddHelpEntry(const std::string& itemId, const std::string& helpText);
    std::string GetHelpText(const std::string& itemId) const;
    std::string GetHelpForItem(const std::string& itemName) const;
    bool HasHelpEntry(const std::string& itemId) const;

    void RegisterWorldTabHelp();
    void RegisterPlayerTabHelp();
    void RegisterTilesTabHelp();
    void RegisterCommonElementsHelp();
    void RegisterButtonHelp(const std::string& buttonName, const std::string& helpId);

    void SetCurrentItem(const std::string& itemId);
    void ResetCurrentItem();

    void RegisterEditorTabHelp();
    void RegisterEditorButtonsHelp();

    void ClearAllEntries();
    size_t GetTotalEntries() const { return m_helpEntries.size(); }

    void RegisterMainMenuHelp();
    void RegisterSaveMenuHelp();
    void RegisterTemplateMenuHelp();

    std::string GetContextHelp(const std::string& context, const std::string& item) const;

private:
    HelpSystem() = default;
    ~HelpSystem() = default;

    std::unordered_map<std::string, std::string> m_helpEntries;
    std::unordered_map<std::string, std::string> m_itemHelpMap;
    std::unordered_map<std::string, std::string> m_buttonHelpMap;
    std::string m_currentItemId;
};