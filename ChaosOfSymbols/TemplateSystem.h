#ifndef TEMPLATESYSTEM_H
#define TEMPLATESYSTEM_H

#include "WorldConfig.h"
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
    bool CreateTemplate(int slot, const std::string& name, const WorldConfig& config);
    bool LoadTemplate(int slot, WorldConfig& config);
    bool DeleteTemplate(int slot);
    bool SaveTemplate(int slot, const WorldConfig& config);

    // Проверки
    bool IsTemplateSlotEmpty(int slot) const;
    bool ClearTemplateDirectory(int slot);

private:
    // Пути
    std::string GetTemplatesDirectory() const;
    std::string GetTemplateSlotPath(int slot) const;

    // Вспомогательные методы
    bool SaveWorldConfig(int slot, const WorldConfig& config);
    bool SavePlayerConfig(int slot, const WorldConfig& config);
    bool SaveTilesConfig(int slot, const WorldConfig& config);
    bool SaveAutomatonConfig(int slot, const WorldConfig& config);
    bool SaveFoodConfig(int slot, const WorldConfig& config);

    bool LoadWorldConfig(int slot, WorldConfig& config);
    bool LoadPlayerConfig(int slot, WorldConfig& config);
    bool LoadTilesConfig(int slot, WorldConfig& config);
    bool LoadAutomatonConfig(int slot, WorldConfig& config);
    bool LoadFoodConfig(int slot, WorldConfig& config);

    bool SaveConfigToFile(const std::string& filePath, const std::string& content);
    bool LoadConfigFromFile(const std::string& filePath,
        std::function<void(const std::string&, const std::string&)> parser);

    std::string GetCurrentDateTime() const;

    // Константы путей
    static const std::string BASE_TEMPLATES_DIR;
    static const std::string TEMPLATE_INFO_FILE;
};

#endif // TEMPLATESYSTEM_H