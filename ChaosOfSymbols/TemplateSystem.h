#ifndef TEMPLATESYSTEM_H
#define TEMPLATESYSTEM_H

#include "WorldEditorConfig.h"
#include <string>
#include <vector>
#include <filesystem>
#include <functional>

namespace fs = std::filesystem;

struct TemplateInfo {
    int slotNumber;
    std::string name;
    std::string creationDate;
    std::string lastModifiedDate;
    std::string templatePath;
    bool isEmpty;

    TemplateInfo() : slotNumber(0), isEmpty(true) {}

    std::string GetDisplayName() const {
        if (isEmpty) return "Empty";
        return name + " (" + creationDate + ")";
    }
};

class TemplateSystem {
public:
    static const int MAX_TEMPLATES = 30;

    TemplateSystem();

    bool Initialize();

    // Получение информации о шаблонах
    std::vector<TemplateInfo> GetTemplates();
    TemplateInfo GetTemplateInfo(int slot) const;

    // Работа с шаблонами
    bool CreateTemplate(int slot, const std::string& name, const WorldEditorConfig& config);
    bool LoadTemplate(int slot, WorldEditorConfig& config);
    bool DeleteTemplate(int slot);
    bool SaveTemplate(int slot, const WorldEditorConfig& config);

    // Проверки
    bool IsTemplateSlotEmpty(int slot) const;

private:
    // Пути
    std::string GetTemplatesDirectory() const;
    std::string GetTemplateSlotPath(int slot) const;

    // Вспомогательные методы
    bool SaveWorldConfig(int slot, const WorldEditorConfig& config);
    bool SavePlayerConfig(int slot, const WorldEditorConfig& config);
    bool SaveTilesConfig(int slot, const WorldEditorConfig& config);
    bool SaveAutomatonConfig(int slot, const WorldEditorConfig& config);
    bool SaveFoodConfig(int slot, const WorldEditorConfig& config);

    bool LoadWorldConfig(int slot, WorldEditorConfig& config);
    bool LoadPlayerConfig(int slot, WorldEditorConfig& config);
    bool LoadTilesConfig(int slot, WorldEditorConfig& config);
    bool LoadAutomatonConfig(int slot, WorldEditorConfig& config);
    bool LoadFoodConfig(int slot, WorldEditorConfig& config);

    bool SaveConfigToFile(const std::string& filePath, const std::string& content);
    bool LoadConfigFromFile(const std::string& filePath,
        std::function<void(const std::string&, const std::string&)> parser);

    std::string GetCurrentDateTime() const;

    // Константы путей
    static const std::string BASE_TEMPLATES_DIR;
    static const std::string TEMPLATE_INFO_FILE;
};

#endif // TEMPLATESYSTEM_H