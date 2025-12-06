//#include "HelpSystem.h"
//#include <algorithm>
//
//HelpSystem& HelpSystem::GetInstance() {
//    static HelpSystem instance;
//    return instance;
//}
//
//void HelpSystem::AddHelpEntry(const std::string& itemId, const std::string& helpText) {
//    m_helpEntries[itemId] = helpText;
//}
//
//std::string HelpSystem::GetHelpText(const std::string& itemId) const {
//    auto it = m_helpEntries.find(itemId);
//    if (it != m_helpEntries.end()) {
//        return it->second;
//    }
//    return "No help available for this item.";
//}
//
//bool HelpSystem::HasHelpEntry(const std::string& itemId) const {
//    return m_helpEntries.find(itemId) != m_helpEntries.end();
//}
//
//void HelpSystem::ClearAllEntries() {
//    m_helpEntries.clear();
//    m_currentItemId.clear();
//}
//
//void HelpSystem::SetCurrentItem(const std::string& itemId) {
//    m_currentItemId = itemId;
//}
//
//std::string HelpSystem::GetCurrentHelpText() const {
//    return GetHelpText(m_currentItemId);
//}
//
//std::string HelpSystem::GetCurrentItemId() const {
//    return m_currentItemId;
//}
//
//void HelpSystem::ResetCurrentItem() {
//    m_currentItemId.clear();
//}