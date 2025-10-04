#pragma once
#include <string>

class GameSettings {
private:
    int m_worldSeed;
    bool m_useRandomSeed;
    std::string m_settingsFilePath;

public:
    GameSettings(const std::string& filePath = "config/settings.cfg");

    bool LoadFromFile();
    bool SaveToFile();
    int GetWorldSeed() const;

    int GetStoredSeed() const { return m_worldSeed; }
    bool UseRandomSeed() const { return m_useRandomSeed; }

    void SetWorldSeed(int seed) { m_worldSeed = seed; }
    void SetUseRandomSeed(bool useRandom) { m_useRandomSeed = useRandom; }
    void SetSettingsFilePath(const std::string& path) { m_settingsFilePath = path; }
};